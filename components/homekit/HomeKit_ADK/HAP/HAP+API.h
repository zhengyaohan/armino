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

#ifndef HAP_API_H
#define HAP_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"
#include "HAPPlatformFeatures.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Compatibility version of the HAP interface.
 *
 * - If this version differs from the one returned by HAPGetCompatibilityVersion,
 *   the library is incompatible and must not be used.
 */
#define HAP_COMPATIBILITY_VERSION (8)

/**
 * Gets the compatibility version of the HAP library.
 *
 * - If the compatibility version differs from HAP_COMPATIBILITY_VERSION,
 *   the library is incompatible and may not be used.
 *
 * @return Compatibility version of the HAP library.
 */
HAP_RESULT_USE_CHECK
uint32_t HAPGetCompatibilityVersion(void);

/**
 * Gets the identification of the HAP library.
 *
 * @return HAP library identification string.
 */
HAP_RESULT_USE_CHECK
const char* HAPGetCompilerVersion(void);

/**
 * Gets the version string of the HAP library.
 *
 * @return Version string of the HAP library.
 */
HAP_RESULT_USE_CHECK
const char* HAPGetVersion(void);

/**
 * Gets the build version string of the HAP library.
 *
 * @return Build version string of the HAP library.
 */
HAP_RESULT_USE_CHECK
const char* HAPGetBuild(void);

/**
 * 128-bit UUID.
 *
 * - The encoding of UUIDs uses reversed byte order compared to RFC 4122, i.e., network byte order backwards.
 *
 * Sample:
 *   UUID: 00112233-4455-6677-8899-AABBCCDDEEFF
 *   bytes: 0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00
 */
typedef struct {
    uint8_t bytes[16]; /**< UUID bytes in reversed network byte order. */
} HAPUUID;
HAP_STATIC_ASSERT(sizeof(HAPUUID) == 16, HAPUUID);
HAP_NONNULL_SUPPORT(HAPUUID)

/**
 * Returns whether or not two UUIDs are equal.
 *
 * @param      uuid                 UUID to compare.
 * @param      otherUUID            UUID to compare with.
 *
 * @return true                     If both UUIDs are equal.
 * @return false                    Otherwise.
 */
HAP_RESULT_USE_CHECK
bool HAPUUIDAreEqual(const HAPUUID* uuid, const HAPUUID* otherUUID);

/**
 * TLV type.
 *
 * - The type semantics depend on the context.
 */
typedef uint8_t HAPTLVType;

/**
 * TLV.
 */
typedef struct {
    /** Value. */
    struct {
        const void* _Nullable bytes; /**< Buffer containing value. */
        size_t numBytes;             /**< Length of buffer. */
    } value;

    /** Type. */
    HAPTLVType type;
} HAPTLV;
HAP_NONNULL_SUPPORT(HAPTLV)

/**
 * TLV reader.
 *
 * Note that applications and PAL should not access the internal fields
 * for implementation detail may change.
 */
typedef struct _HAPTLVReader {
    /**@cond */
    void* _Nullable bytes; /**< Buffer containing TLV data. Modified while reading. */
    size_t numBytes;       /**< Length of data in buffer. */
    size_t maxBytes;       /**< Capacity of buffer. */

    /**
     * Non-sequential access to TLVs requires reservation of a few TLV types to keep track of state.
     * Those TLV types should be picked carefully to not conflict with TLV types that are of interest.
     * TLVs using those types will be skipped and ignored. They will not be available for reading.
     *
     * When non-sequential access is turned on, TLV values are no longer implicitly NULL terminated
     * unless opted in explicitly when fetching each TLV. NULL terminated TLVs are not allowed
     * to contain NULL values themselves.
     *
     * Even with non-sequential access turned on each TLV may only be read once.
     * However, there are no restrictions on the number of times un-read TLVs may be enumerated.
     *
     * Implementation details:
     * - Data is processed in-place. No allocations take place. This is especially important
     *   w.r.t. sub-TLV structures where no free space is available before or after the TLV.
     * - When a TLV is read, potential fragmented data is merged and the TLV value is relocated
     *   according to the provided NULL terminator choice. The original TLV type is overwritten
     *   to mark the TLV as read, and to keep track of the format in which the TLV value is encoded.
     * - The different TLV value encodings are as follows: (<field : numBytes>, NULL are 0-bytes)
     *   1. Single-fragment value that may contain NULL values, but is not NULL terminated.
     *      - <type : 1>
     *      - <numValueBytes : 1>
     *      - <value : N>
     *   2. Single-fragment value that may not contain NULL values, but is NULL terminated.
     *      - <type : 1>
     *      - <value : N>
     *      - <NULL : 1>
     *   3. Multi-fragment value that may contain NULL values, and also is NULL terminated.
     *      - <type : 1>
     *      - <numFragments - 2 : X>
     *      - <numLastFragmentBytes : 1>
     *      - <NULL : 2 * (numFragments - 2) - (X - 1)>
     *      - <value : N>
     *      - <NULL : 1>
     *      The second field <numFragments - 2 : X> is a variable-length integer format
     *      where all byte values are summed up and including the first non-255 byte.
     *      X denotes the number of bytes used for this representation.
     *      For example, 1000 would be encoded as <255> <255> <255> <235> and X is 4,
     *      or 510 would be encoded as <255> <255> <0> and X is 3.
     * - When non-sequential access is turned on all TLV items are enumerated,
     *   and if the data contains TLVs with a TLV type that has a special meaning,
     *   those TLVs are implicitly read without NULL terminators and discarded.
     *   This ensures that after this initial enumeration it can be relied on that TLVs
     *   with types that have a special meaning always denote data that has already been processed.
     * - When a TLV with a type that has a special meaning is found after the initial enumeration
     *   the TLV is skipped as it has already been processed without modifying its value.
     *   This means that once a TLV has been read the client can assume that its value is stable
     *   and won't be moved to other memory locations even as other TLVs are accessed.
     */
    /**@{*/
    struct {
        HAPTLVType singleFragment;
        HAPTLVType nullTerminatedSingleFragment;
        HAPTLVType nullTerminatedMultiFragment;
    } tlvTypes;
    bool isNonSequentialAccessEnabled : 1;
    /**@}*/
    /**@endcond */
} HAPTLVReader;

/**
 * Creates a new TLV reader.
 *
 * @param      reader               An uninitialized TLV reader.
 * @param      bytes                Buffer containing raw TLV data. Will be modified by the reader. Must remain valid.
 * @param      numBytes             Length of buffer.
 */
void HAPTLVReaderCreate(HAPTLVReader* reader, void* _Nullable bytes, size_t numBytes);

/**
 * Fetches the next TLV item from a TLV reader's buffer. TLV item content is NULL-terminated for convenience.
 *
 * @param      reader               Reader to fetch TLV item from.
 * @param[out] found                True if a TLV item has been fetched. False otherwise.
 * @param[out] tlv                  Next TLV item. Valid when @p found is true. Fragments are merged automatically.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed (Incomplete item, or violation of TLV rules).
 */
HAP_RESULT_USE_CHECK
HAPError HAPTLVReaderGetNext(HAPTLVReader* reader, bool* found, HAPTLV* tlv);

/**
 * Fetches TLV items by type into @p tlvs.
 *
 * On input, @p tlvs is a NULL-terminated array to TLV items.
 * For each TLV item, the type shall be specified. Types must be unique.
 *
 * On output, @p tlvs are updated to contain the actual TLV items.
 * If multiple TLV items with one of the requested types are found, an error is returned.
 * TLV item contents are NULL-terminated for convenience.
 *
 * - This API must be used on a freshly initialized TLV reader. All TLV items will be read.
 *
 * @param     reader                Reader to fetch TLV items from.
 * @param[in,out] tlvs              NULL-terminated array to TLV structures.
 *                                  On input, type filters are specified. On output, actual TLV items are filled in.
 *                                  For TLV items not found, values are set to NULL.
 *                                  Each type may only be requested once.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidData    If parsing failed (Incomplete item, or violation of TLV rules), or if multiple TLV
 *                                  items with a requested type are found.
 */
HAP_RESULT_USE_CHECK
HAPError HAPTLVReaderGetAll(HAPTLVReader* reader, HAPTLV* _Nullable const* _Nonnull tlvs);

/**
 * TLV Writer.
 */
struct _HAPTLVWriter;
typedef struct _HAPTLVWriter HAPTLVWriter;
HAP_NONNULL_SUPPORT(HAPTLVWriter)

/**
 * Creates a new TLV writer.
 *
 * @param      writer               An uninitialized TLV writer.
 * @param      bytes                Buffer to serialize TLV data into. Must remain valid.
 * @param      maxBytes             Capacity of the buffer.
 */
void HAPTLVWriterCreate(HAPTLVWriter* writer, void* bytes, size_t maxBytes);

/**
 * Resets a TLV writer.
 *
 * @param      writer               An initialized TLV writer to reset.
 */
void HAPTLVWriterReset(HAPTLVWriter* writer);

/**
 * Serializes a TLV item and appends it to the writer's buffer.
 *
 * - Multiple items of same type must be separated by an item with different type. Use 0x00 as the TLV delimiter for all
 *   characteristics unless otherwise specified.
 *
 * @param      writer               Writer to append serialized TLV to.
 * @param      tlv                  TLV to write.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized TLV item.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 18.1.1 TLV Rules
 */
HAP_RESULT_USE_CHECK
HAPError HAPTLVWriterAppend(HAPTLVWriter* writer, const HAPTLV* tlv);

/**
 * Retrieves the buffer containing the serialized TLV data.
 *
 * @param      writer               Writer to retrieve buffer from.
 * @param[out] bytes                Buffer containing serialized TLV data written so far.
 * @param[out] numBytes             Length of buffer.
 */
void HAPTLVWriterGetBuffer(const HAPTLVWriter* writer, void* _Nonnull* _Nonnull bytes, size_t* numBytes);

/**
 * Retrieves the buffer containing the serialized TLV data.
 *
 * @param      writer               Writer to retrieve buffer from.
 * @param[out] bytes                Buffer containing serialized TLV data written so far.
 * @param[out] numBytes             Length of buffer.
 */
void HAPTLVWriterPrintBuffer(const HAPTLVWriter* writer, char* writerName);

/**
 * Retrieves a temporary buffer of unused memory, e.g., to prepare the next TLV payload to be written.
 *
 * - /!\ The buffer becomes invalid after writing to the TLV writer again.
 *
 * @param      writer               Writer to retrieve buffer from.
 * @param[out] scratchBytes         Temporary buffer of free memory.
 * @param[out] numScratchBytes      Capacity of scratch buffer.
 */
void HAPTLVWriterGetScratchBytes(
        const HAPTLVWriter* writer,
        void* _Nonnull* _Nonnull scratchBytes,
        size_t* numScratchBytes);

/**
 * Retrieves a temporary buffer of unused memory, e.g., to prepare the next TLV payload to be written.
 *
 * The buffer returned will be as big as possible within the writer's buffer space to hold one TLV
 * value (in other words, the buffer reserves bytes in the writer to hold space for the TLV headers
 * and possible fragmentation).
 *
 * - /!\ The buffer becomes invalid after writing to the TLV writer again.
 *
 * @param      writer               Writer to retrieve buffer from.
 * @param[out] scratchBytes         Temporary buffer of free memory.
 * @param[out] numScratchBytes      Capacity of scratch buffer.
 */
void HAPTLVWriterGetScratchBytesForTLVValue(
        const HAPTLVWriter* _Nonnull writer,
        void* _Nonnull* _Nonnull scratchBytes,
        size_t* _Nonnull numScratchBytes);

/**
 * Extends the writer's buffer with serialized data.
 *
 * @param      writer               Writer to extend
 * @param      bytes                bytes to extend the writer with
 * @param      numBytes             number of bytes
 * @param      lastType             last TLV type of the extended bytes
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the writer's buffer is not large enough to hold the serialized TLV item.
 */
HAP_RESULT_USE_CHECK
HAPError HAPTLVWriterExtend(HAPTLVWriter* writer, void* _Nonnull bytes, size_t numBytes, HAPTLVType lastType);

/**
 * Create a sub-writer to write TLV's inside an outer TLV.
 *
 * @param      subWriter           The new subWriter.
 * @param      outerWriter         The outer writer that contains this subWriter.
 */
void HAPTLVCreateSubWriter(HAPTLVWriter* _Nonnull subWriter, HAPTLVWriter* _Nonnull outerWriter);

/**
 * Finish a sub-writer and write it into the outer writer with the given type.
 *
 * - /!\ Do not use the subWriter again after finalizing (except to create a new one in its place).
 *
 * @param      subWriter           The new subWriter.
 * @param      outerWriter         The outer writer that contains this subWriter.
 * @param      tlvType             The TLV type that wraps the inner values.
 */
HAPError HAPTLVFinalizeSubWriter(HAPTLVWriter* subWriter, HAPTLVWriter* outerWriter, HAPTLVType tlvType);

/**
 * HomeKit Accessory server.
 */
struct _HAPAccessoryServer;
typedef struct _HAPAccessoryServer HAPAccessoryServer;
HAP_NONNULL_SUPPORT(HAPAccessoryServer)

/**
 * HomeKit Session.
 */
struct _HAPSession;
typedef struct _HAPSession HAPSession;
HAP_NONNULL_SUPPORT(HAPSession)

/**
 * HomeKit Data Stream.
 */
union _HAPDataStreamRef;
typedef union _HAPDataStreamRef HAPDataStreamRef;
HAP_NONNULL_SUPPORT(HAPDataStreamRef)

/**
 * Maximum size of a HomeKit Data Stream payload.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 9.1.2 Frame Format
 */
#define kHAPDataStream_MaxPayloadBytes ((size_t) HAPMin(((uint32_t) 1U << 20U) - 1U, SIZE_MAX))

/**
 * Formats that HomeKit characteristics can have.
 *
 * - IMPORTANT: The format must always match the type of the characteristic structure!
 *   For example, kHAPCharacteristicFormat_UInt8 may only be used on HAPUInt8Characteristic.
 *
 * - For Apple-defined characteristics, the format must be used exactly as defined in the specification.
 *   For example, in the specification the Brightness characteristic is defined to have type "int".
 *   Therefore, a HAPIntCharacteristic must be used for it (with format kHAPCharacteristicFormat_Int).
 *
 * - For vendor-specific characteristics, any format may be chosen.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicFormat) { /**
                                                    * Opaque data blob. Raw bytes.
                                                    *
                                                    * - Default format in case no other format has been specified.
                                                    */
                                                   kHAPCharacteristicFormat_Data,

                                                   /**
                                                    * Boolean. True or false.
                                                    */
                                                   kHAPCharacteristicFormat_Bool,

                                                   /**
                                                    * Unsigned 8-bit integer.
                                                    */
                                                   kHAPCharacteristicFormat_UInt8,

                                                   /**
                                                    * Unsigned 16-bit integer.
                                                    */
                                                   kHAPCharacteristicFormat_UInt16,

                                                   /**
                                                    * Unsigned 32-bit integer.
                                                    */
                                                   kHAPCharacteristicFormat_UInt32,

                                                   /**
                                                    * Unsigned 64-bit integer.
                                                    */
                                                   kHAPCharacteristicFormat_UInt64,

                                                   /**
                                                    * Signed 32-bit integer.
                                                    */
                                                   kHAPCharacteristicFormat_Int,

                                                   /**
                                                    * 32-bit floating point.
                                                    */
                                                   kHAPCharacteristicFormat_Float,

                                                   /**
                                                    * UTF-8 string.
                                                    */
                                                   kHAPCharacteristicFormat_String,

                                                   /**
                                                    * One or more TLV8s.
                                                    */
                                                   kHAPCharacteristicFormat_TLV8
} HAP_ENUM_END(uint8_t, HAPCharacteristicFormat);

/**
 * Properties that HomeKit characteristics can have.
 *
 * For Apple-defined characteristics, the default values for the following properties are defined by the specification:
 * - readable (Paired Read)
 * - writable (Paired Write)
 * - supportsEventNotification (Notify)
 *
 * The remaining properties have to be evaluated on a case by case basis.
 */
typedef struct {
    /**
     * The characteristic is readable.
     *
     * - A read handler must be plugged into the characteristic structure's corresponding callback field.
     *   Only controllers with a secured connection can perform reads.
     */
    bool readable : 1;

    /**
     * The characteristic is writable.
     *
     * - A write handler must be plugged into the characteristic structure's corresponding callback field.
     *   Only controllers with a secured connection can perform writes.
     */
    bool writable : 1;

    /**
     * The characteristic supports notifications using the event connection established by the controller.
     * The event connection provides unidirectional communication from the accessory to the controller.
     *
     * - A read handler must be plugged into the characteristic structure's corresponding callback field.
     *   Only controllers with a secured connection can subscribe to events.
     *
     * - When the characteristic state changes, the HAPAccessoryServerRaiseEvent or
     *   HAPAccessoryServerRaiseEventOnSession function must be called.
     */
    bool supportsEventNotification : 1;

    /**
     * The characteristic should be hidden from the user.
     *
     * - This is useful for characteristics to configure the accessory or to update firmware on the accessory.
     *   Generic HomeKit applications on the controller won't show these characteristics.
     *
     * - When all characteristics in a service are marked hidden then the service must also be marked as hidden.
     */
    bool hidden : 1;

    /**
     * The characteristic will only be accessible for read operations by admin controllers.
     *
     * - Reads to the characteristic will only execute if the controller has admin permissions.
     *
     * - Event notification values for the characteristic will only be delivered to controllers with admin permissions.
     *
     * - The subscription state of event notifications for the characteristic will only be modified by controllers with
     *   admin permissions.
     */
    bool readRequiresAdminPermissions : 1;

    /**
     * The characteristic will only be accessible for write operations by admin controllers.
     *
     * - Writes to the characteristic will only execute if the controller has admin permissions.
     */
    bool writeRequiresAdminPermissions : 1;

    /**
     * The characteristic requires time sensitive actions.
     *
     * - Writes to the characteristic will only execute if the accessory can be reached within a short time frame.
     *   This is useful for example for security class characteristics like Lock Target State or Target Door State.
     *
     * - A write handler must be plugged into the characteristic structure's corresponding callback field.
     *   The characteristic must also be marked as writable.
     */
    bool requiresTimedWrite : 1;

    /**
     * The characteristic requires additional authorization data.
     *
     * - Additional authorization data is controller-provided data that the accessory may use to validate that the
     *   controller is authorized to perform a requested operation. The contents of the authorization data are
     *   manufacturer specific. The additional authorization data is provided by the accessory app (an iOS app
     *   provided by the accessory manufacturer to control or configure the accessory) to iOS and stored by iOS
     *   on the controller. The additional authorization data must not be unique per write request as the controller
     *   will not construct or receive unique authorization data for each request. Additional authorization data
     *   may change periodically, e.g., once per month, or when user permissions change.
     *
     * - A write handler must be plugged into the characteristic structure's corresponding callback field.
     *   The characteristic must also be marked as writable.
     *
     * - It is left up to the write handler to validate additional authorization data.
     *   If authorization is insufficient, kHAPError_NotAuthorized must be returned from the handler.
     *
     * @see iOS documentation for [HMCharacteristic updateAuthorizationData(_:completionHandler:)]
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 2.3.3.2 Additional Authorization Data
     */
    bool supportsAuthorizationData : 1;
    /**
     * The characteristic supports reading additional context data during reads and event notifications
     *
     * - This context information is provided by the app during a write request, or by the device itself
     *   via other means.
     * - The context information is read out by one of the two ContextCallbacks if the characteristic and
     *   characteristic type supports it.
     * - In order to support event notification context information, handleReadContextData must be implemented.
     * - handleReadContextDataCompact is optional.
     */
    bool supportsEventNotificationContextInformation : 1;
    /**
     * These properties only affect connections over IP (Ethernet / Wi-Fi).
     *
     * - If the accessory only supports Bluetooth, these properties can be ignored.
     */
    struct {
        /**
         * This flag prevents the characteristic from being read during discovery.
         *
         * - Normally, all characteristic values are being read during discovery over an IP transport.
         *   If a characteristic maintains state across multiple reads / writes, depending on the timing
         *   such discovery operations may interfere with correct operation.
         *
         * - Setting this flag prevents the characteristic from being read during discovery.
         *   Instead, a NULL value will be sent to the controller during discovery. Only explicit reads are processed.
         *
         * - This property may be useful for characteristics that handle firmware updates of the accessory.
         */
        bool controlPoint : 1;

        /**
         * Write operations on the characteristic require a read response value.
         *
         * - This property is typically applicable to write control point operations.
         *
         * - Write and read handlers must be plugged into the characteristic structure's corresponding callback fields.
         *   The characteristic must also be marked as writable.
         *
         * - After a successful write, a read operation is always requested immediately (i.e., without any other
         *   requests in-between).
         */
        bool supportsWriteResponse : 1;
    } ip;

    /**
     * These properties only affect connections over Bluetooth LE.
     *
     * - If the accessory only supports IP (Ethernet, Wi-Fi), these properties can be ignored.
     */
    struct {
        /**
         * The characteristic supports broadcast notifications. Such broadcasts happen when the characteristic state
         * changes while no controller is connected. This allows paired controllers to quickly react to the notification
         * without having to re-establish a secured connection.
         *
         * - When this property is not set, controllers may only receive state updates while connected.
         *
         * - A read handler must be plugged into the characteristic structure's corresponding callback field.
         *   Only paired controllers can decode and process the broadcasts.
         *
         * - When the characteristic state changes, the HAPAccessoryServerRaiseEvent or
         *   HAPAccessoryServerRaiseEventOnSession function must be called.
         */
        bool supportsBroadcastNotification : 1;

        /**
         * The characteristic supports disconnected notifications. When the characteristic state changes while no
         * controller is connected, paired controllers will re-establish a secured connection to the accessory and
         * fetch the updated characteristic state.
         *
         * - When this property is set, the following properties must also be set:
         *   - readable
         *   - supportsEventNotification
         *   - ble.supportsBroadcastNotification
         *
         * - This property must be set on at least one characteristic of an accessory to work around an issue
         *   in certain versions of the Home app that would otherwise claim that Additional Setup is required.
         *
         * - Disconnected events should only be used to reflect important state changes in the accessory.
         *   For example, contact sensor state changes or current door state changes should use this property.
         *   On the other hand, a temperature sensor must not use this property for changes in temperature readings.
         *
         * - A read handler must be plugged into the characteristic structure's corresponding callback field.
         *
         * - When the characteristic state changes, the HAPAccessoryServerRaiseEvent or
         *   HAPAccessoryServerRaiseEventOnSession function must be called.
         */
        bool supportsDisconnectedNotification : 1;

        /**
         * The characteristic is always readable, even before a secured session is established.
         *
         * - This is mainly an internal property that is used on characteristics that handle the pairing process.
         *
         * - A read handler must be plugged into the characteristic structure's corresponding callback field.
         */
        bool readableWithoutSecurity : 1;

        /**
         * The characteristic is always writable, even before a secured session is established.
         *
         * - This is mainly an internal property that is used on characteristics that handle the pairing process.
         *
         * - A write handler must be plugged into the characteristic structure's corresponding callback field.
         */
        bool writableWithoutSecurity : 1;
    } ble;
} HAPCharacteristicProperties;
HAP_STATIC_ASSERT(sizeof(HAPCharacteristicProperties) == 4, HAPCharacteristicProperties);

/**
 * Units that numeric HomeKit characteristics can have.
 *
 * For Apple-defined characteristics, the corresponding units are defined by the specification.
 */
HAP_ENUM_BEGIN(uint8_t, HAPCharacteristicUnits) { /**
                                                   * Unitless. Used for example on enumerations.
                                                   */
                                                  kHAPCharacteristicUnits_None,

                                                  /**
                                                   * The unit of the characteristic is degrees celsius.
                                                   */
                                                  kHAPCharacteristicUnits_Celsius,

                                                  /**
                                                   * The unit of the characteristic is the degrees of an arc.
                                                   */
                                                  kHAPCharacteristicUnits_ArcDegrees,

                                                  /**
                                                   * The unit of the characteristic is a percentage %.
                                                   */
                                                  kHAPCharacteristicUnits_Percentage,

                                                  /**
                                                   * The unit of the characteristic is lux (that is, illuminance).
                                                   */
                                                  kHAPCharacteristicUnits_Lux,

                                                  /**
                                                   * The unit of the characteristic is seconds.
                                                   */
                                                  kHAPCharacteristicUnits_Seconds
} HAP_ENUM_END(uint8_t, HAPCharacteristicUnits);

/**
 * Transport type over which a request has been received or over which a response will be sent.
 */
HAP_ENUM_BEGIN(uint8_t, HAPTransportType) {
    /**
     * HAP over IP (Ethernet / Wi-Fi).
     */
    kHAPTransportType_IP = 1,

    /**
     * HAP over Bluetooth LE.
     */
    kHAPTransportType_BLE,

    /**
     * HAP over Thread
     */
    kHAPTransportType_Thread,
} HAP_ENUM_END(uint8_t, HAPTransportType);

typedef void HAPCharacteristic;
typedef struct HAPService HAPService;
typedef struct HAPAccessory HAPAccessory;
/**
 * Callbacks related to event notification context information.
 */
typedef struct {
    /*
     * Used to supply context information for normal reads and connected event notifications
     */
    HAP_RESULT_USE_CHECK
    HAPError (*_Nullable handleReadContextData)(
            HAPAccessoryServer* server,
            const HAPCharacteristic* characteristic,
            HAPTLVWriter* _Nullable* _Nullable writer,
            void* _Nullable context);

    /*
     * Used to supply context information for broadcast notification and other space limited cases
     * Optional
     */
    HAP_RESULT_USE_CHECK
    HAPError (*_Nullable handleReadContextDataCompact)(
            HAPAccessoryServer* server,
            const HAPCharacteristic* characteristic,
            uint8_t* buffer,
            size_t bufferSize,
            void* _Nullable context);
} HAPCharacteristicContextCallbacks;

// <editor-fold desc="HAPDataCharacteristic">

typedef struct HAPDataCharacteristic HAPDataCharacteristic;

/**
 * Data characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPDataCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPDataCharacteristicReadRequest;

/**
 * Data characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPDataCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPDataCharacteristicWriteRequest;

/**
 * Data characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPDataCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPDataCharacteristicSubscriptionRequest;

/**
 * HomeKit Data characteristic.
 */
struct HAPDataCharacteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_Data.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * Value constraints.
     */
    struct {
        uint32_t maxLength; /**< Maximum length. */
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in the value buffer is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] valueBytes           Value buffer.
         * @param      maxValueBytes        Capacity of value buffer.
         * @param[out] numValueBytes        Length of value buffer.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPDataCharacteristicReadRequest* request,
                void* valueBytes,
                size_t maxValueBytes,
                size_t* numValueBytes,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      valueBytes           Value buffer.
         * @param      numValueBytes        Length of value buffer.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPDataCharacteristicWriteRequest* request,
                const void* valueBytes,
                size_t numValueBytes,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPDataCharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPDataCharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPBoolCharacteristic">

typedef struct HAPBoolCharacteristic HAPBoolCharacteristic;

/**
 * Bool characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPBoolCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPBoolCharacteristicReadRequest;

/**
 * Bool characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPBoolCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPBoolCharacteristicWriteRequest;

/**
 * Bool characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPBoolCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPBoolCharacteristicSubscriptionRequest;

/**
 * HomeKit Bool characteristic.
 */
struct HAPBoolCharacteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_Bool.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPBoolCharacteristicReadRequest* request,
                bool* value,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      val                  Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPBoolCharacteristicWriteRequest* request,
                bool value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPBoolCharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPBoolCharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPUInt8Characteristic">

typedef struct HAPUInt8Characteristic HAPUInt8Characteristic;

/**
 * UInt8 characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPUInt8Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt8CharacteristicReadRequest;

/**
 * UInt8 characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPUInt8Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
    /**
     * Additional context data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw context data, if applicable. */
        size_t numBytes;             /**< Length of context data. */
    } contextData;
} HAPUInt8CharacteristicWriteRequest;

/**
 * UInt8 characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPUInt8Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt8CharacteristicSubscriptionRequest;

/**
 * Valid Values range for a UInt8 characteristic.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 7.4.5.3 Valid Values Range Descriptor
 */
typedef struct {
    uint8_t start; /**< Starting value. */
    uint8_t end;   /**< Ending value. */
} HAPUInt8CharacteristicValidValuesRange;

/**
 * HomeKit UInt8 characteristic.
 */
struct HAPUInt8Characteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_UInt8.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;
    HAPCharacteristicContextCallbacks contextCallbacks;
    /**
     * The units of the values for the characteristic.
     */
    HAPCharacteristicUnits units;

    /**
     * Value constraints.
     */
    struct {
        uint8_t minimumValue; /**< Minimum value. */
        uint8_t maximumValue; /**< Maximum value. */
        uint8_t stepValue;    /**< Step value. */

        /**
         * List of valid values in ascending order. NULL-terminated. Optional.
         *
         * - Only supported for Apple defined characteristics.
         *
         * @see HomeKit Accessory Protocol Specification R17
         *      Section 7.4.5.2 Valid Values Descriptor
         */
        const uint8_t* _Nullable const* _Nullable validValues;

        /**
         * List of valid values ranges in ascending order. NULL-terminated. Optional.
         *
         * - Only supported for Apple defined characteristics.
         *
         * @see HomeKit Accessory Protocol Specification R17
         *      Section 7.4.5.3 Valid Values Range Descriptor
         */
        const HAPUInt8CharacteristicValidValuesRange* _Nullable const* _Nullable validValuesRanges;
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPUInt8CharacteristicReadRequest* request,
                uint8_t* value,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      val                  Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPUInt8CharacteristicWriteRequest* request,
                uint8_t value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt8CharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt8CharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPUInt16Characteristic">

typedef struct HAPUInt16Characteristic HAPUInt16Characteristic;

/**
 * UInt16 characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPUInt16Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt16CharacteristicReadRequest;

/**
 * UInt16 characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPUInt16Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPUInt16CharacteristicWriteRequest;

/**
 * UInt16 characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPUInt16Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt16CharacteristicSubscriptionRequest;

/**
 * HomeKit UInt16 characteristic.
 */
struct HAPUInt16Characteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_UInt16.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * The units of the values for the characteristic.
     */
    HAPCharacteristicUnits units;

    /**
     * Value constraints.
     */
    struct {
        uint16_t minimumValue; /**< Minimum value. */
        uint16_t maximumValue; /**< Maximum value. */
        uint16_t stepValue;    /**< Step value. */
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPUInt16CharacteristicReadRequest* request,
                uint16_t* value,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      val                  Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPUInt16CharacteristicWriteRequest* request,
                uint16_t value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt16CharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt16CharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPUInt32Characteristic">

typedef struct HAPUInt32Characteristic HAPUInt32Characteristic;

/**
 * UInt32 characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPUInt32Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt32CharacteristicReadRequest;

/**
 * UInt32 characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPUInt32Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPUInt32CharacteristicWriteRequest;

/**
 * UInt32 characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPUInt32Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt32CharacteristicSubscriptionRequest;

/**
 * HomeKit UInt32 characteristic.
 */
struct HAPUInt32Characteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_UInt32.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * The units of the values for the characteristic.
     */
    HAPCharacteristicUnits units;

    /**
     * Value constraints.
     */
    struct {
        uint32_t minimumValue; /**< Minimum value. */
        uint32_t maximumValue; /**< Maximum value. */
        uint32_t stepValue;    /**< Step value. */
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPUInt32CharacteristicReadRequest* request,
                uint32_t* value,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      val                  Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPUInt32CharacteristicWriteRequest* request,
                uint32_t value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt32CharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt32CharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPUInt64Characteristic">

typedef struct HAPUInt64Characteristic HAPUInt64Characteristic;

/**
 * UInt64 characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPUInt64Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt64CharacteristicReadRequest;

/**
 * UInt64 characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPUInt64Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPUInt64CharacteristicWriteRequest;

/**
 * UInt64 characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPUInt64Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPUInt64CharacteristicSubscriptionRequest;

/**
 * HomeKit UInt64 characteristic.
 */
struct HAPUInt64Characteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_UInt64.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * The units of the values for the characteristic.
     */
    HAPCharacteristicUnits units;

    /**
     * Value constraints.
     */
    struct {
        uint64_t minimumValue; /**< Minimum value. */
        uint64_t maximumValue; /**< Maximum value. */
        uint64_t stepValue;    /**< Step value. */
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPUInt64CharacteristicReadRequest* request,
                uint64_t* value,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      val                  Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPUInt64CharacteristicWriteRequest* request,
                uint64_t value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt64CharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPUInt64CharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPIntCharacteristic">

typedef struct HAPIntCharacteristic HAPIntCharacteristic;

/**
 * Int characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPIntCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPIntCharacteristicReadRequest;

/**
 * Int characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPIntCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPIntCharacteristicWriteRequest;

/**
 * Int characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPIntCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPIntCharacteristicSubscriptionRequest;

/**
 * HomeKit Int characteristic.
 */
struct HAPIntCharacteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_Int.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * The units of the values for the characteristic.
     */
    HAPCharacteristicUnits units;

    /**
     * Value constraints.
     */
    struct {
        int32_t minimumValue; /**< Minimum value. */
        int32_t maximumValue; /**< Maximum value. */
        int32_t stepValue;    /**< Step value. */
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPIntCharacteristicReadRequest* request,
                int32_t* value,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      val                  Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPIntCharacteristicWriteRequest* request,
                int32_t value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPIntCharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPIntCharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPFloatCharacteristic">

typedef struct HAPFloatCharacteristic HAPFloatCharacteristic;

/**
 * Float characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPFloatCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPFloatCharacteristicReadRequest;

/**
 * Float characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPFloatCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPFloatCharacteristicWriteRequest;

/**
 * Float characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPFloatCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPFloatCharacteristicSubscriptionRequest;

/**
 * HomeKit Float characteristic.
 */
struct HAPFloatCharacteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_Float.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * The units of the values for the characteristic.
     */
    HAPCharacteristicUnits units;

    /**
     * Value constraints.
     */
    struct {
        float minimumValue; /**< Minimum value. */
        float maximumValue; /**< Maximum value. */
        float stepValue;    /**< Step value. */
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPFloatCharacteristicReadRequest* request,
                float* value,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      val                  Value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPFloatCharacteristicWriteRequest* request,
                float value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPFloatCharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPFloatCharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPStringCharacteristic">

typedef struct HAPStringCharacteristic HAPStringCharacteristic;

/**
 * String characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPStringCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPStringCharacteristicReadRequest;

/**
 * String characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPStringCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPStringCharacteristicWriteRequest;

/**
 * String characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPStringCharacteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPStringCharacteristicSubscriptionRequest;

/**
 * HomeKit String characteristic.
 */
struct HAPStringCharacteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_String.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * Value constraints.
     */
    struct {
        uint16_t maxLength; /**< Maximum length. */
    } constraints;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if the characteristic is marked readable in the characteristic properties.
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param[out] value                Value. NULL-terminated.
         * @param      maxValueBytes        Capacity of value. NULL-terminator must fit within capacity.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPStringCharacteristicReadRequest* request,
                char* value,
                size_t maxValueBytes,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      value                Value. NULL-terminated.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPStringCharacteristicWriteRequest* request,
                const char* value,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPStringCharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPStringCharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>
// <editor-fold desc="HAPTLV8Characteristic">

typedef struct HAPTLV8Characteristic HAPTLV8Characteristic;

/**
 * TLV8 characteristic read request.
 */
typedef struct {
    /**
     * Transport type over which the response will be sent.
     */
    HAPTransportType transportType;

    /**
     * The session over which the response will be sent.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     * - May be NULL if a request is generated internally (e.g., for BLE broadcasts while disconnected).
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* _Nullable session;

    /**
     * The characteristic whose value is to be read.
     */
    const HAPTLV8Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPTLV8CharacteristicReadRequest;

/**
 * TLV8 characteristic write request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be written.
     */
    const HAPTLV8Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;

    /**
     * Additional authorization data.
     */
    struct {
        const void* _Nullable bytes; /**< Raw AAD data, if applicable. */
        size_t numBytes;             /**< Length of additional authorization data. */
    } authorizationData;
} HAPTLV8CharacteristicWriteRequest;

/**
 * TLV8 characteristic subscription request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     */
    HAPSession* session;

    /**
     * The characteristic whose value is to be subscribed or unsubscribed.
     */
    const HAPTLV8Characteristic* characteristic;

    /**
     * The service that contains the characteristic.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPTLV8CharacteristicSubscriptionRequest;

/**
 * HomeKit TLV8 characteristic.
 */
struct HAPTLV8Characteristic {
    /**
     * Format. Must be kHAPCharacteristicFormat_TLV8.
     */
    HAPCharacteristicFormat format;

    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the characteristic.
     */
    const HAPUUID* characteristicType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * Description of the characteristic provided by the manufacturer of the accessory.
     */
    const char* _Nullable manufacturerDescription;

    /**
     * Characteristic properties.
     */
    HAPCharacteristicProperties properties;

    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to handle read requests, notifications, and write responses.
         * On success, the value stored in @p value is sent back to the controller.
         *
         * - Required if any of the following characteristic properties are marked:
         *   - readable
         *   - supportsEventNotification
         *   - supportsWriteResponse
         *   - supportsBroadcastNotification
         *   - supportsDisconnectedNotification
         *   - readableWithoutSecurity
         * - The callback must not block. Consider prefetching values if it would take too long.
         * - The returned value must satisfy the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      responseWriter       TLV writer for serializing the response.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleRead)(
                HAPAccessoryServer* server,
                const HAPTLV8CharacteristicReadRequest* request,
                HAPTLVWriter* responseWriter,
                void* _Nullable context);

        /**
         * The callback used to handle write requests.
         *
         * - Required if the characteristic is marked writeable in the characteristic properties.
         * - The callback must not block. Consider queueing values if it would take too long.
         * - The value is already checked against the constraints of the characteristic.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      requestReader        TLV reader for parsing the value.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_NotAuthorized  If additional authorization data is insufficient.
         * @return kHAPError_Busy           If the request failed temporarily.
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable handleWrite)(
                HAPAccessoryServer* server,
                const HAPTLV8CharacteristicWriteRequest* request,
                HAPTLVReader* requestReader,
                void* _Nullable context);

        /**
         * The callback used to handle subscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleSubscribe)(
                HAPAccessoryServer* server,
                const HAPTLV8CharacteristicSubscriptionRequest* request,
                void* _Nullable context);

        /**
         * The callback used to handle unsubscribe requests.
         *
         * - The callback must not block. Consider queueing values if it would take too long.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         */
        void (*_Nullable handleUnsubscribe)(
                HAPAccessoryServer* server,
                const HAPTLV8CharacteristicSubscriptionRequest* request,
                void* _Nullable context);
    } callbacks;
};

// </editor-fold>

/**
 * Properties that HomeKit services can have.
 *
 * - If service properties are enabled, a Service Signature characteristic must be attached to the service.
 *   This is only necessary if the accessory supports Bluetooth LE, but also okay for IP accessories.
 */
typedef struct {
    /**
     * The service is the primary service on the accessory.
     *
     * - Only one service may be marked as primary.
     *
     * - The primary service must be an Apple-defined service.
     *   Custom services must not be advertised as the primary service.
     */
    bool primaryService : 1;

    /**
     * The service should be hidden from the user.
     *
     * - This is useful for services to configure the accessory or to update firmware on the accessory.
     *   Generic HomeKit applications on the controller won't show these services.
     *
     * - When all characteristics in a service are marked hidden then the service must also be marked as hidden.
     */
    bool hidden : 1;

    /**
     * These properties only affect connections over Bluetooth LE.
     *
     * - If the accessory only supports IP (Ethernet / Wi-Fi), these properties can be ignored.
     */
    struct {
        /**
         * The service supports configuration.
         *
         * - This must be set on the HAP Protocol Information service.
         * - This must not be set on other services.
         */
        bool supportsConfiguration : 1;
    } ble;
} HAPServiceProperties;
HAP_STATIC_ASSERT(sizeof(HAPServiceProperties) == 2, HAPServiceProperties);

/**
 * Service request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The service that is being accessed.
     */
    const HAPService* service;

    /**
     * The accessory that provides the service.
     */
    const HAPAccessory* accessory;
} HAPServiceRequest;

/**
 * HomeKit service.
 */
struct HAPService {
    /**
     * Instance ID.
     *
     * - Must be unique across all service or characteristic instance IDs of the accessory.
     * - Must not change while the accessory is paired, including over firmware updates.
     * - Must be 1 for the Accessory Information Service.
     * - Must not be 0.
     * - For accessories that support Bluetooth LE, must not exceed UINT16_MAX.
     */
    uint64_t iid;

    /**
     * The type of the service.
     */
    const HAPUUID* serviceType;

    /**
     * Description for debugging (based on "Type" field of HAP specification).
     */
    const char* debugDescription;

    /**
     * The name of the service.
     *
     * - Must be set if the service provides user visible state or interaction.
     * - Must not be set for user invisible services like a Firmware Update service.
     * - The user may adjust the name on the controller. Such changes are local only and won't sync to the accessory.
     *
     * - If a name is set, a Name characteristic must be attached to the service.
     */
    const char* _Nullable name;

    /**
     * HAP Service properties.
     *
     * - Only one service may be marked as primary.
     * - If all characteristics in a service are marked as hidden, the service must also be marked as hidden.
     *
     * - If service properties are enabled, a Service Signature characteristic must be attached to the service.
     *   This is only necessary if the accessory supports Bluetooth LE, but also okay for IP accessories.
     */
    HAPServiceProperties properties;

    /**
     * Array containing instance IDs of linked services. 0-terminated.
     *
     * - Links are not transitive. If A links to B and B links to C, it is not implied that A links to C as well.
     * - Services may not link to themselves.
     *
     * - If linked services are used, a Service Signature characteristic must be attached to the service.
     *   This is only necessary if the accessory supports Bluetooth LE, but also okay for IP accessories.
     */
    const uint16_t* _Nullable linkedServices;

    /**
     * Array of contained characteristics. NULL-terminated.
     *
     * - All HAPCharacteristic structures can be used in this array.
     *   Please ensure that the "format" field of each structure is correct!
     */
    const HAPCharacteristic* _Nullable const* _Nullable characteristics;
};
HAP_NONNULL_SUPPORT(HAPService)

/**
 * Accessory category.
 *
 * - An accessory with support for multiple categories should advertise the primary category. An accessory for which a
 *   primary category cannot be determined or the primary category isn't among the well defined categories falls in the
 *   `Other` category.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 16 Accessory Categories
 */
HAP_ENUM_BEGIN(uint8_t, HAPAccessoryCategory) {
    /** Accessory that is accessed through a bridge. */
    kHAPAccessoryCategory_BridgedAccessory = 0,

    /** Other. */
    kHAPAccessoryCategory_Other = 1,

    /** Bridges. */
    kHAPAccessoryCategory_Bridges = 2,

    /** Fans. */
    kHAPAccessoryCategory_Fans = 3,

    /**
     * Garage Door Openers.
     *
     * - This accessory category must use programmable tags if NFC is supported.
     */
    kHAPAccessoryCategory_GarageDoorOpeners = 4,

    /** Lighting. */
    kHAPAccessoryCategory_Lighting = 5,

    /**
     * Locks.
     *
     * - This accessory category must use programmable tags if NFC is supported.
     */
    kHAPAccessoryCategory_Locks = 6,

    /** Outlets. */
    kHAPAccessoryCategory_Outlets = 7,

    /** Switches. */
    kHAPAccessoryCategory_Switches = 8,

    /** Thermostats. */
    kHAPAccessoryCategory_Thermostats = 9,

    /** Sensors. */
    kHAPAccessoryCategory_Sensors = 10,

    /**
     * Security Systems.
     *
     * - This accessory category must use programmable tags if NFC is supported.
     */
    kHAPAccessoryCategory_SecuritySystems = 11,

    /**
     * Doors.
     *
     * - This accessory category must use programmable tags if NFC is supported.
     */
    kHAPAccessoryCategory_Doors = 12,

    /**
     * Windows.
     *
     * - This accessory category must use programmable tags if NFC is supported.
     */
    kHAPAccessoryCategory_Windows = 13,

    /** Window Coverings. */
    kHAPAccessoryCategory_WindowCoverings = 14,

    /** Programmable Switches. */
    kHAPAccessoryCategory_ProgrammableSwitches = 15,

    /**
     * Range Extenders.
     *
     * @remark Obsolete since R10.
     *
     * @see HomeKit Accessory Protocol Specification R9
     *      Table 12-3 Accessory Categories
     */
    kHAPAccessoryCategory_RangeExtenders = 16,

    /**
     * IP Cameras.
     *
     * - This accessory category must use programmable tags if NFC is supported.
     */
    kHAPAccessoryCategory_IPCameras = 17,

    /**
     * Video Doorbells.
     *
     * - This accessory category must use programmable tags if NFC is supported.
     */
    kHAPAccessoryCategory_VideoDoorbells = 18,

    /** Air Purifiers. */
    kHAPAccessoryCategory_AirPurifiers = 19,

    /** Heaters. */
    kHAPAccessoryCategory_Heaters = 20,

    /** Air Conditioners. */
    kHAPAccessoryCategory_AirConditioners = 21,

    /** Humidifiers. */
    kHAPAccessoryCategory_Humidifiers = 22,

    /** Dehumidifiers. */
    kHAPAccessoryCategory_Dehumidifiers = 23,

    /**
     * Apple TV.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 11.116 Target Control List
     */
    kHAPAccessoryCategory_AppleTV = 24,

    /** Sprinklers. */
    kHAPAccessoryCategory_Sprinklers = 28,

    /** Faucets. */
    kHAPAccessoryCategory_Faucets = 29,

    /** Shower Systems. */
    kHAPAccessoryCategory_ShowerSystems = 30,

    /** Remotes. */
    kHAPAccessoryCategory_Remotes = 32,

    /** Wi-Fi Routers. */
    kHAPAccessoryCategory_WiFiRouters = 33,

} HAP_ENUM_END(uint8_t, HAPAccessoryCategory);

/**
 * Request that originated the HomeKit Data Stream.
 */
typedef HAPServiceRequest HAPDataStreamRequest;

/**
 * HomeKit Data Stream callbacks.
 */
typedef struct {
    /**
     * The callback used to handle accepted HomeKit Data Streams.
     *
     * @param      server               Accessory server.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           The newly accepted HomeKit Data Stream.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleAccept)(
            HAPAccessoryServer* server,
            const HAPDataStreamRequest* request,
            HAPDataStreamRef* dataStream,
            void* _Nullable context);

    /**
     * The callback used when a HomeKit Data Stream is invalidated.
     *
     * - /!\ WARNING: The HomeKit Data Stream must no longer be used after this callback returns.
     *
     * @param      server               Accessory server.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           HomeKit Data Stream.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleInvalidate)(
            HAPAccessoryServer* server,
            const HAPDataStreamRequest* request,
            HAPDataStreamRef* dataStream,
            void* _Nullable context);

    /**
     * The callback used to handle incoming data.
     *
     * - Use a series of HAPDataStreamReceiveData and HAPDataStreamSkipData calls to receive or skip data parts.
     *   Total requested length must match the total length of incoming data.
     *
     * - /!\ WARNING: Received data must only be processed after all parts have been successfully received or skipped.
     *   If a decryption error occurs, an error will be set only after the final part is received.
     *
     * @param      server               Accessory server.
     * @param      request              Request that originated the HomeKit Data Stream.
     * @param      dataStream           HomeKit Data Stream.
     * @param      totalDataBytes       Total length of incoming data.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleData)(
            HAPAccessoryServer* server,
            const HAPDataStreamRequest* request,
            HAPDataStreamRef* dataStream,
            size_t totalDataBytes,
            void* _Nullable context);
} HAPDataStreamCallbacks;

/**
 * Invalidates a HomeKit Data Stream.
 *
 * - When invalidation is complete, the handleInvalidate callback will be called.
 *
 * - This also cancels all pending HomeKit Data Stream tasks (completion handlers will be called with an error).
 *
 * - /!\ WARNING: The HomeKit Data Stream must no longer be used after this function returns.
 *
 * @param      server               Accessory server.
 * @param      dataStream           HomeKit Data Stream.
 */
void HAPDataStreamInvalidate(HAPAccessoryServer* server, HAPDataStreamRef* dataStream);

/**
 * Completion handler of a HomeKit Data Stream operation.
 *
 * @param      server               Accessory server.
 * @param      request              Request that originated the HomeKit Data Stream.
 * @param      dataStream           HomeKit Data Stream.
 * @param      error                kHAPError_None           If successful.
 *                                  kHAPError_InvalidState   If the HomeKit Data Stream is being invalidated.
 *                                                           No further operations may be started.
 * @param      dataBytes            Data buffer provided when starting the HomeKit Data Stream operation.
 * @param      numDataBytes         Number of processed bytes.
 * @param      isComplete           An indication that all data parts have been processed.
 * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
 */
typedef void (*HAPDataStreamDataCompletionHandler)(
        HAPAccessoryServer* server,
        const HAPDataStreamRequest* request,
        HAPDataStreamRef* dataStream,
        HAPError error,
        void* _Nullable dataBytes,
        size_t numDataBytes,
        bool isComplete,
        void* _Nullable context);

/**
 * Receives data from a HomeKit Data Stream.
 *
 * - When the handleData callback is invoked, use a series of HAPDataStreamReceiveData and HAPDataStreamSkipData calls
 *   to receive or skip data parts. Total requested length must match the total length of incoming data.
 *
 * - /!\ WARNING: Received data must only be processed after all parts have been successfully received or skipped.
 *   If a decryption error occurs, an error will be set only after the final part is received.
 *
 * - /!\ WARNING: The data buffer must remain valid until the completion handler is invoked.
 *
 * - Only one receive or skip operation can be active at a time on a HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataBytes            Data buffer to fill with incoming data. Must remain valid.
 * @param      numDataBytes         Number of bytes to read.
 * @param      completionHandler    Completion handler to call when the task is complete.
 */
void HAPDataStreamReceiveData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        void* dataBytes,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler);

/**
 * Skips incoming data from a HomeKit Data Stream.
 *
 * - When the handleData callback is invoked, use a series of HAPDataStreamReceiveData and HAPDataStreamSkipData calls
 *   to receive or skip data parts. Total requested length must match the total length of incoming data.
 *
 * - Only one receive or skip operation can be active at a time on a HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dataStream           HomeKit Data Stream.
 * @param      numDataBytes         Number of bytes to skip.
 * @param      completionHandler    Completion handler to call when the task is complete.
 */
void HAPDataStreamSkipData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler);

/**
 * Prepares to send data over a HomeKit Data Stream.
 *
 * - When the completion handler is invoked, use a series of HAPDataStreamSendData / HAPDataStreamSendMutableData calls
 *   to send data parts. Total sent length must match the total length of outgoing data.
 *
 * - Only one send operation can be active at a time on a HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dataStream           HomeKit Data Stream.
 * @param      totalDataBytes       Total length of outgoing data.
 * @param      completionHandler    Completion handler to call when the task is complete.
 */
void HAPDataStreamPrepareData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        size_t totalDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler);

/**
 * Sends data over a HomeKit Data Stream.
 *
 * - This function is less efficient than the HAPDataStreamSendMutableData function. If possible, use the latter.
 *
 * - Before this function can be called, use HAPDataStreamPrepareData to provide the length of data.
 *   Then, use a series of HAPDataStreamSendData / HAPDataStreamSendMutableData calls to send data parts.
 *   Total sent length must match the total length of outgoing data.
 *
 * - /!\ WARNING: The data buffer must remain valid until the completion handler is invoked.
 *
 * - Only one send operation can be active at a time on a HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataBytes            Data buffer. Must remain valid.
 * @param      numDataBytes         Number of bytes to write.
 * @param      completionHandler    Completion handler to call when the task is complete.
 */
void HAPDataStreamSendData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        const void* dataBytes,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler);

/**
 * Sends mutable data over a HomeKit Data Stream.
 *
 * - Before this function can be called, use HAPDataStreamPrepareData to provide the length of data.
 *   Then, use a series of HAPDataStreamSendData / HAPDataStreamSendMutableData calls to send data parts.
 *   Total sent length must match the total length of outgoing data.
 *
 * - /!\ WARNING: The data buffer must remain valid until the completion handler is invoked.
 *
 * - Only one send operation can be active at a time on a HomeKit Data Stream.
 *
 * @param      server               Accessory server.
 * @param      dataStream           HomeKit Data Stream.
 * @param      dataBytes            Data buffer. Will be modified. Must remain valid.
 * @param      numDataBytes         Number of bytes to write.
 * @param      completionHandler    Completion handler to call when the task is complete.
 */
void HAPDataStreamSendMutableData(
        HAPAccessoryServer* server,
        HAPDataStreamRef* dataStream,
        void* dataBytes,
        size_t numDataBytes,
        HAPDataStreamDataCompletionHandler completionHandler);

/**
 * Accessory identify request.
 */
typedef struct {
    /**
     * Transport type over which the request has been received.
     */
    HAPTransportType transportType;

    /**
     * The session over which the request has been received.
     *
     * - A controller may be logged in on multiple sessions concurrently.
     *
     * - For remote requests (e.g., via Apple TV), the associated controller may not be
     *   the one who originated the request, but instead the admin controller who set up the Apple TV.
     */
    HAPSession* session;

    /**
     * The accessory that is being accessed.
     */
    const HAPAccessory* accessory;

    /**
     * Whether the request appears to have originated from a remote controller, e.g., via Apple TV.
     */
    bool remote;
} HAPAccessoryIdentifyRequest;

/**
 * Accessory firmware update state.
 */
typedef struct {
    /**
     * Update duration (seconds).
     *
     * - The amount of time the accessory is expected to be unresponsive while applying an update. The duration covers
     *   the time from which the apply request is received until the accessory is available for communication again.
     */
    uint16_t updateDuration;

    /**
     * The current state of the firmware update process.
     *
     * - For state definitions, reference HAPCharacteristicValue_FirmwareUpdateStatus_FirmwareUpdateState.
     */
    uint8_t updateState;

    /**
     * Staging not ready reason.
     *
     * - If cleared, the accessory is ready to stage a firmware update. For bit definitions, reference
     *   HAPCharacteristicValue_FirmwareUpdateReadiness_StagingNotReadyReason.
     */
    uint32_t stagingNotReadyReason;

    /**
     * Update not ready reason.
     *
     * - If cleared, the accessory is ready to apply a firmware update. For bit definitions, reference
     *   HAPCharacteristicValue_FirmwareUpdateReadiness_UpdateNotReadyReason.
     */
    uint32_t updateNotReadyReason;

    /**
     * Staged firmware version.
     *
     * - The staged version string must conform to the same format as that required by the Firmware Revision
     *   characteristic. If there is no staged firmware, this pointer must be NULL.
     */
    const char* _Nullable stagedFirmwareVersion;
} HAPAccessoryFirmwareUpdateState;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
/**
 * Diagnostics Snapshot format
 */
HAP_ENUM_BEGIN(uint8_t, HAPDiagnosticsSnapshotFormat) {
    /** Zip. */
    kHAPDiagnosticsSnapshotFormat_Zip = 0,
    /** Text. */
    kHAPDiagnosticsSnapshotFormat_Text = 1,
} HAP_ENUM_END(uint8_t, HAPDiagnosticsSnapshotFormat);

/**
 * Diagnostics Snapshot type
 */
HAP_ENUM_BEGIN(uint8_t, HAPDiagnosticsSnapshotType) {
    /** Manufacturer. */
    kHAPDiagnosticsSnapshotType_Manufacturer = (uint8_t)(1U << 0U),

    /** ADK. */
    kHAPDiagnosticsSnapshotType_ADK = (uint8_t)(1U << 1U),
} HAP_ENUM_END(uint8_t, HAPDiagnosticsSnapshotType);

/**
 * Diagnostics Snapshot Options
 */
HAP_ENUM_BEGIN(uint8_t, HAPDiagnosticsSnapshotOptions) {
    /** None. */
    kHAPDiagnosticsSnapshotOptions_None = (uint8_t)(0U << 0U),

    /** Configurable max log size. */
    kHAPDiagnosticsSnapshotOptions_ConfigurableMaxLogSize = (uint8_t)(1U << 1U)
} HAP_ENUM_END(uint8_t, HAPDiagnosticsSnapshotOptions);

/**
 * Accessory diagnostics information.
 */
typedef struct {
    /**
     * Diagnostics Snapshot format.
     *
     * - The format to be used for the diagnostics data.
     */
    HAPDiagnosticsSnapshotFormat diagnosticsSnapshotFormat;

    /**
     * Diagnostics Snapshot type.
     *
     * - The type of the diagnostics data.
     */
    HAPDiagnosticsSnapshotType diagnosticsSnapshotType;

    /**
     * Diagnostics Snapshot options.
     *
     * - The diagnostics options.
     */
    HAPDiagnosticsSnapshotOptions diagnosticsSnapshotOptions;

    /**
     * Supported Diagnostics Modes.
     *
     * Available logging modes on the accessory.
     */
    uint32_t diagnosticsSupportedMode;

    /**
     * Diagnostics context.
     *
     * - Diagnostics context to be allocated by the accessory
     */
    const void* diagnosticsContext;

} HAPAccessoryDiagnosticsConfig;
#endif // HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)

typedef struct {
    uint32_t sleepIntervalInMs;
} HAPAccessoryReachabilityConfiguration;

/**
 * Completion handler of an IP Camera getSupportedRecordingConfiguration request.
 *
 * @param      server               Accessory server.
 * @param      service              Camera Event Recording Management service, or a linked service.
 * @param      accessory            Camera accessory.
 * @param      supportedConfig      Supported recording configuration.
 */
typedef void (*HAPCameraAccessoryGetSupportedRecordingConfigurationCompletionHandler)(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        const HAPCameraRecordingSupportedConfiguration* supportedConfig);

/**
 * Max firmware version string length.
 */
#define kHAPFirmwareVersion_MaxLength ((size_t) 64)

/**
 * HomeKit accessory.
 */
struct HAPAccessory {
    /**
     * Accessory instance ID.
     *
     * For regular accessories (Bluetooth LE / IP):
     * - Must be 1.
     *
     * For bridged accessories:
     * - Must be unique for the bridged accessory and not change across firmware updates or power cycles.
     */
    uint64_t aid;

    /**
     * Category information for the accessory.
     *
     * For regular accessories (Bluetooth LE / IP):
     * - Must match the functionality of the accessory's primary service.
     *
     * For bridged accessories:
     * - Must be kHAPAccessoryCategory_BridgedAccessory.
     */
    HAPAccessoryCategory category;

    /**
     * The display name of the accessory.
     *
     * - Maximum length 64 (excluding NULL-terminator).
     * - must start and end with a letter or number with the only exception of ending with a "." (period).
     * - may have special characters " - " (dashes), " " " (quotes), " ' " (apostrophe), " , " (comma),
     *   " . " (period), " # " (hash) and " & " (ampersand) only.
     * - ':' and ';' characters should not be used for accessories that support Bluetooth LE.
     * - The user may adjust the name on the controller. Such changes are local only and won't sync to the accessory.
     */
    const char* name;

    /**
     * Product Data information assigned to each Product Plan on the MFi Portal upon Product Plan submission.
     *
     * For regular accessories (Bluetooth LE / IP):
     * - Product Data (8 bytes) must be encoded as a 16 character hex string (lowercase zero padded). For example,
     *   if the assigned Product Data is 0x03d8a775e3644573, then the encoded hex string is "03d8a775e3644573".
     *
     * For bridged accessories:
     * - Product Data may be NULL.
     */
    const char* _Nullable productData;

    /**
     * The manufacturer of the accessory.
     *
     * - Maximum length 64 (excluding NULL-terminator).
     */
    const char* manufacturer;

    /**
     * The model name of the accessory.
     *
     * - Minimum length 1 (excluding NULL-terminator).
     * - Maximum length 64 (excluding NULL-terminator).
     */
    const char* model;

    /**
     * The serial number of the accessory.
     *
     * - Minimum length 2 (excluding NULL-terminator).
     * - Maximum length 64 (excluding NULL-terminator).
     */
    const char* serialNumber;

    /**
     * The firmware version of the accessory.
     *
     * - x[.y[.z]] (e.g., "100.1.1")
     * - Each number must not be greater than UINT32_MAX.
     * - Maximum length 64 (excluding NULL-terminator).
     */
    const char* firmwareVersion;

    /**
     * The hardware version of the accessory.
     *
     * - x[.y[.z]] (e.g., "100.1.1")
     * - Maximum length 64 (excluding NULL-terminator).
     */
    const char* _Nullable hardwareVersion;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
    /**
     * The RGB color value of the hardware finish of the accessory.
     * - Bits 0-7 defines the B component
     * - Bits 8-15 defines the G component
     * - Bits 16-23 defines the R component
     * - Bits 24-31 are reserved
     */
    uint32_t hardwareFinish;
#endif

    /**
     * Array of provided services. NULL-terminated.
     */
    const HAPService* _Nullable const* _Nullable services;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
    /**
     * HomeKit Data Stream specific options.
     *
     * - This is only necessary if the accessory supports HomeKit Data Stream.
     */
    struct {
        /**
         * Delegate to handle HomeKit Data Stream events.
         */
        struct {
            /** Callbacks. */
            const HAPDataStreamCallbacks* _Nullable callbacks;

            /** Client context pointer. */
            void* _Nullable context;
        } delegate;
    } dataStream;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
    /**
     * Array of IP Camera stream configurations. NULL-terminated.
     *
     * - For every available camera stream, a stream configuration must be specified
     *   and a Camera RTP Stream Management Service must be added to the services array.
     *
     * - Only supported for HAP over IP (Ethernet / Wi-Fi).
     */
    const HAPCameraStreamSupportedConfigurations* _Nullable const* _Nullable cameraStreamConfigurations;
#endif

    /**
     * Accessory reliability Configuration
     */
    HAPAccessoryReachabilityConfiguration* _Nullable reachabilityConfiguration;
    /**
     * Callbacks.
     */
    struct {
        /**
         * The callback used to invoke the identify routine.
         *
         * - All accessories (except television accessories) must implement an identify routine,
         *   a means of identifying the accessory so that it can be located by the user.
         *   The identify routine should run no longer than five seconds.
         * - The accessory must implement an identify routine, a means of identifying the accessory
         *   so that it can be located by the user. The identify routine should run no longer than five seconds.
         *
         * @param      server               Accessory server.
         * @param      request              Request.
         * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
         *
         * @return kHAPError_None           If successful.
         * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
         * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
         * @return kHAPError_InvalidData    If the controller sent a malformed request.
         * @return kHAPError_OutOfResources If out of resources to process request.
         * @return kHAPError_Busy           If the request failed temporarily.
         * @see HomeKit Accessory Protocol Specification
         *      Section 6.7.6 Identify Routine
         */
        HAP_RESULT_USE_CHECK
        HAPError (*_Nullable identify)(
                HAPAccessoryServer* server,
                const HAPAccessoryIdentifyRequest* request,
                void* _Nullable context);

#if HAP_FEATURE_ENABLED(HAP_FEATURE_CAMERA)
        /**
         * IP Camera specific callbacks.
         */
        struct {
            /**
             * The callback used to get the supported recording configuration.
             *
             * - completionHandler must be called before this callback returns.
             *
             * @param      server               Accessory server.
             * @param      service              Camera Event Recording Management service.
             * @param      accessory            Camera accessory.
             * @param      completionHandler    Completion handler.
             * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
             */
            void (*_Nullable getSupportedRecordingConfiguration)(
                    HAPAccessoryServer* server,
                    const HAPService* service,
                    const HAPAccessory* accessory,
                    HAPCameraAccessoryGetSupportedRecordingConfigurationCompletionHandler completionHandler,
                    void* _Nullable context);
        } camera;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_UARP_SUPPORT)
        /**
         * Firmware update specific callbacks.
         */
        struct {
            /**
             * The callback used to get the accessory firmware update state.
             *
             * @param      server               Accessory server.
             * @param      accessory            Accessory being accessed.
             * @param      accessoryState       Pointer to state structure.
             * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
             *
             * @return kHAPError_None           If successful.
             * @return kHAPError_Busy           If the request failed temporarily.
             * @return kHAPError_Unknown        If unable to retrieve accessory firmware update state.
             */
            HAP_RESULT_USE_CHECK
            HAPError (*_Nullable getAccessoryState)(
                    HAPAccessoryServer* server,
                    const HAPAccessory* accessory,
                    HAPAccessoryFirmwareUpdateState* accessoryState,
                    void* _Nullable context);
        } firmwareUpdate;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_DIAGNOSTICS_SERVICE)
        /**
         * Diagnostics Configuration specific callback
         */
        struct {
            /**
             * The callback used to get the diagnostics configuration from the accessory.
             *
             * @param      server                           Accessory server.
             * @param      accessory                        Accessory being accessed.
             * @param      accessoryDiagnosticsConfig       Accessory diagnostics configuration
             * @param      context                          The context parameter given to the HAPAccessoryServerCreate
             * function.
             *
             * @return kHAPError_None                       If successful.
             * @return kHAPError_Busy                       If the request failed temporarily.
             * @return kHAPError_Unknown                    If unable to retrieve accessory firmware update state.
             */

            HAP_RESULT_USE_CHECK
            HAPError (*_Nullable getDiagnosticsConfig)(
                    HAPAccessoryServer* _Nullable server,
                    const HAPAccessory* _Nullable accessory,
                    HAPAccessoryDiagnosticsConfig* accessoryDiagnosticsConfig,
                    void* _Nullable context);
        } diagnosticsConfig;
#endif

        /**
         * Reachability specific callbacks
         */
        struct {
            /**
             * The callback used to notify the accessory of a controller ping event.
             *
             * @param      accessory              Accessory being accessed.
             * @param      session                HAP session associated with the event.
             */
            void (*_Nullable handleControllerPing)(const HAPAccessory* accessory, const HAPSession* session);
        } reachability;

        /**
         * Siri specific callbacks
         */
        struct {
            /**
             * The callback used to get the type of Siri Input used by the accessory.
             *
             * @param       server                 Accessory server.
             * @param[out]  value                  Representation of the Siri Input type.
             * @param       context                The context parameter given to the HAPAccessoryServerCreate
             * function.
             *
             * @return kHAPError_None           If successful.
             * @return kHAPError_Unknown        If unable to perform operation with requested service or characteristic.
             * @return kHAPError_InvalidState   If the request cannot be processed in the current state.
             * @return kHAPError_InvalidData    If the controller sent a malformed request.
             * @return kHAPError_OutOfResources If out of resources to process request.
             * @return kHAPError_Busy           If the request failed temporarily.
             */
            HAP_RESULT_USE_CHECK
            HAPError (*_Nullable getSiriInputType)(
                    HAPAccessoryServer* server,
                    uint8_t* value,
                    void* _Nullable context HAP_UNUSED);
        } siri;
    } callbacks;
};
HAP_NONNULL_SUPPORT(HAPAccessory)

/**
 * Returns the IP Camera provider for an accessory.
 *
 * @param      server               Accessory server.
 * @param      accessory            Camera accessory.
 *
 * @return IP Camera provider for the given accessory.
 */
HAPPlatformCameraRef HAPAccessoryGetCamera(HAPAccessoryServer* server, const HAPAccessory* accessory);

/**
 * Denotes what parts of the supported IP Camera recording configuration did change.
 */
HAP_OPTIONS_BEGIN(uint8_t, HAPCameraSupportedRecordingConfigurationChange) {
    /** Supported Camera Recording Configuration did change. */
    kHAPCameraSupportedRecordingConfigurationChange_Camera = 1U << 0U,

    /** Supported Video Recording Configuration did change. */
    kHAPCameraSupportedRecordingConfigurationChange_Video = 1U << 1U,

    /** Supported Audio Recording Configuration did change. */
    kHAPCameraSupportedRecordingConfigurationChange_Audio = 1U << 2U
} HAP_OPTIONS_END(uint8_t, HAPCameraSupportedRecordingConfigurationChange);

/**
 * Handles a change of the supported IP Camera recording configuration.
 *
 * - The new IP Camera recording configuration will be requested using the getSupportedRecordingConfiguration callback.
 *   If the currently selected recording configuration is no longer supported, it will be invalidated.
 *
 * @param      server               Accessory server.
 * @param      service              Camera Event Recording Management service, or a linked service.
 * @param      accessory            Camera accessory.
 * @param      changes              What parts of the supported IP Camera recording configuration did change.
 */
void HAPCameraAccessoryHandleSupportedRecordingConfigurationChange(
        HAPAccessoryServer* server,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPCameraSupportedRecordingConfigurationChange changes);

/**
 * Minimum number of supported pairings.
 *
 * - Pairings are stored in one of the domains of the platform's key-value store.
 *   The maximum number of supported pairings is restricted to the available number of keys per domain (256).
 */
#define kHAPPairingStorage_MinElements ((HAPPlatformKeyValueStoreKey) 16)

/**
 * IP read context.
 */
struct _HAPIPReadContext;
typedef struct _HAPIPReadContext HAPIPReadContext;

/**
 * IP write context.
 */
struct _HAPIPWriteContext;
typedef struct _HAPIPWriteContext HAPIPWriteContext;

/**
 * IP session descriptor.
 */
struct _HAPIPSessionDescriptor;
typedef struct _HAPIPSessionDescriptor HAPIPSessionDescriptor;

/**
 * IP event notification.
 */
struct _HAPIPEventNotification;
typedef struct _HAPIPEventNotification HAPIPEventNotification;

/**
 * Default size for the inbound buffer of an IP session.
 */
#define kHAPIPSession_DefaultInboundBufferSize ((size_t) 32768)

/**
 * Default size for the outbound buffer of an IP session.
 */
#define kHAPIPSession_DefaultOutboundBufferSize ((size_t) 32768)

/**
 * Default size for the scratch buffer of an IP session.
 */
#define kHAPIPSession_DefaultScratchBufferSize ((size_t) 8192)

/**
 * IP session.
 *
 * - For accessories that support IP (Ethernet / Wi-Fi), one of these structures needs to be allocated
 *   per concurrently supported IP connection, and provided as part of a HAPIPSessionStorage structure.
 *
 * - The provided memory must remain valid while the accessory server is initialized.
 */
struct _HAPIPSession;
typedef struct _HAPIPSession HAPIPSession;

/**
 * Default number of elements in a HAPIPSessionStorage.
 * The value must be maximum number of simultaneous sessions plus one, in order to store one extra session
 * being destroyed while accepting a new one.
 */
#define kHAPIPSessionStorage_DefaultNumElements ((size_t) 9)

// Per HAP spec, 8 simultaneous sessions must be supported.
// Because one more extra session storage is used for a session being destroyed while accepting a new
// one, storage for at least 9 sessions is required.
HAP_STATIC_ASSERT(kHAPIPSessionStorage_DefaultNumElements >= 9, kHAPIPSessionStorage_DefaultNumElements);

/**
 * IP server storage.
 *
 * - For accessories that support IP (Ethernet / Wi-Fi), one of these structures needs to be allocated
 *   and provided as part of the initialization options in HAPAccessoryServerCreate.
 *
 * - The provided memory (including the HAPIPServerStorage structure) must remain valid
 *   while the accessory server is initialized.
 */
typedef struct {
    /**
     * IP sessions.
     *
     * - One session must be provided per concurrently supported IP connection.
     *   Each session contains additional memory that needs to be allocated. See HAPIPSession.
     *
     * - At least eight elements are required for IP (Ethernet / Wi-Fi) accessories.
     */
    HAPIPSession* sessions;

    /**
     * Number of sessions.
     */
    size_t numSessions;

    /**
     * IP read contexts.
     *
     * - At least one of these structures must be allocated per HomeKit characteristic and service and must remain
     *   valid while the accessory server is initialized.
     */
    HAPIPReadContext* readContexts;

    /**
     * Number of read contexts.
     */
    size_t numReadContexts;

    /**
     * IP write contexts.
     *
     * - At least one of these structures must be allocated per HomeKit characteristic and service and must remain
     *   valid while the accessory server is initialized.
     */
    HAPIPWriteContext* writeContexts;

    /**
     * Number of write contexts.
     */
    size_t numWriteContexts;

    /**
     * Scratch buffer.
     */
    struct {
        /**
         * Scratch buffer. Memory must be allocated and must remain valid while the accessory server is initialized.
         *
         * - It is recommended to allocate at least kHAPIPSession_DefaultScratchBufferSize bytes,
         *   but the optimal size may vary depending on the accessory's attribute database.
         */
        void* bytes;

        /**
         * Size of scratch buffer.
         */
        size_t numBytes;
    } scratchBuffer;

#if (HAP_DYNAMIC_MEMORY_ALLOCATION == 1)
    /**
     * Dynamic memory allocation (optional).
     *
     * - Unless a static accessory server storage configuration is provided, functions for dynamic memory allocation
     *   have to be provided in this structure.
     */
    struct {
        /**
         * Pointer to platform-specific function used to dynamically allocate memory.
         *
         * - The provided memory allocation function must implement the same interface and semantics as the function
         *   malloc() in the ISO C standard.
         *
         * @param      numBytes             Number of bytes that shall be allocated.
         *
         * @return Upon successful completion with parameter numBytes not equal to 0, allocateMemory() shall return
         *         a pointer to the allocated memory object. If numBytes is 0, either a NULL pointer or a unique
         *         pointer that can be successfully passed to deallocateMemory() shall be returned. Otherwise,
         *         allocateMemory() shall return a NULL pointer.
         */
        HAP_RESULT_USE_CHECK
        void* _Nullable (*_Nullable allocateMemory)(size_t numBytes);

        /**
         * Pointer to platform-specific function used to dynamically reallocate memory.
         *
         * - The provided memory reallocation function must implement the same interface and semantics as the function
         *   realloc() in the ISO C standard.
         *
         * @param      bytes                Pointer to the previously allocated memory object that shall be
         *                                  reallocated.
         * @param      numBytes             New number of bytes that shall be allocated.
         *
         * @return Upon successful completion with parameter numBytes not equal to 0, reallocateMemory() shall
         *         return a pointer to the (possibly moved) allocated memory object. If numBytes is 0, either a NULL
         *         pointer or a unique pointer that can be successfully passed to deallocateMemory() shall be
         *         returned. If there is not enough available memory, the original memory object is left unchanged
         *         reallocateMemory() shall return a NULL pointer.
         */
        HAP_RESULT_USE_CHECK
        void* _Nullable (*_Nullable reallocateMemory)(void* _Nullable bytes, size_t numBytes);

        /**
         * Pointer to platform-specific function used to dynamically deallocate memory.
         *
         * - The provided memory deallocation function must implement the same interface and semantics as the function
         *   free() in the ISO C standard.
         *
         * @param      bytes                Pointer to the previously allocated memory object that shall be
         *                                  deallocated.
         */
        void (*_Nullable deallocateMemory)(void* _Nullable bytes);
    } dynamicMemoryAllocation;
#endif
} HAPIPAccessoryServerStorage;
HAP_NONNULL_SUPPORT(HAPIPAccessoryServerStorage)

/**
 * IP Camera streaming session.
 *
 * - For IP Camera accessories, one of these structures needs to be allocated per Camera RTP Stream Management Service
 *   and provided as part of a HAPCameraStreamingSessionStorage structure.
 */
struct _HAPCameraStreamingSession;
typedef struct _HAPCameraStreamingSession HAPCameraStreamingSession;

/**
 * IP Camera streaming session setup.
 *
 * - For IP Camera accessories, one of these structures needs to be allocated per Camera RTP Stream Management Service
 *   and concurrently supported IP connection, and provided as part of a HAPCameraStreamingSessionStorage structure.
 */
struct _HAPCameraStreamingSessionSetup;
typedef struct _HAPCameraStreamingSessionSetup HAPCameraStreamingSessionSetup;

/**
 * IP Camera streaming sessions.
 *
 * - For IP Camera accessories, one of these structures needs to be allocated
 *   and provided as part of the initialization options in HAPAccessoryServerCreate.
 *
 * - The provided streaming sessions must remain valid while the accessory server is initialized.
 */
typedef struct {
    /**
     * IP Camera streaming sessions.
     *
     * - One session must be provided per Camera RTP Stream Management Service.
     */
    HAPCameraStreamingSession* sessions;

    /**
     * Number of sessions.
     */
    size_t numSessions;

    /**
     * IP Camera streaming session setups.
     *
     * - One setup must be provided per concurrently supported IP connection
     *   and per Camera RTP Stream Management Service.
     */
    HAPCameraStreamingSessionSetup* setups;

    /**
     * Number of setups.
     */
    size_t numSetups;
} HAPCameraStreamingSessionStorage;
HAP_NONNULL_SUPPORT(HAPCameraStreamingSessionStorage)

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
/**
 * Access Code operation type
 */
HAP_ENUM_BEGIN(uint8_t, HAPAccessCodeOperationType) {
    /**
     * Enumerates access code identifiers
     */
    kHAPAccessCodeOperationType_EnumerateIdentifiers,

    /**
     * Read an access code data.
     *
     * Read operaiton must succeed even if it's called from within enumeration callback,
     * or even if it's called while bulk operations are queued.
     *
     * Read operation will read values as to be modified if it is called
     * while bulk operations are queued.
     */
    kHAPAccessCodeOperationType_Read,

    /**
     * Start bulk operations
     */
    kHAPAccessCodeOperationType_BulkOperationStart,

    /**
     * Commit queued bulk operations
     */
    kHAPAccessCodeOperationType_BulkOperationCommit,

    /**
     * Abort queued bulk operations if any
     */
    kHAPAccessCodeOperationType_BulkOperationAbort,

    /**
     * Queue access code add operation as part of bulk operation
     */
    kHAPAccessCodeOperationType_QueueAdd,

    /**
     * Queue access code update operation as part of bulk operation
     */
    kHAPAccessCodeOperationType_QueueUpdate,

    /**
     * Queue access code remove operation as part of bulk operation
     */
    kHAPAccessCodeOperationType_QueueRemove,
} HAP_ENUM_END(uint8_t, HAPAccessCodeOperationType);

/**
 * Callback function for identifier enumeration.
 *
 * @param identifier   identifier of an access code
 * @param ctx          context
 */
typedef void (*HAPAccessCodeIdentifierCallback)(uint32_t identifier, void* _Nullable ctx);

/**
 * Access Code operation data
 *
 * Note that pointer fields that are set as a result of an operation must stay valid
 * till next operation call.
 * For instance, if identifier field is set to a string as a result of queue-add operation,
 * the identifier string must stay valid till another operation is called.
 */
typedef struct {
    /**
     * Operation type
     */
    HAPAccessCodeOperationType type;

    /**
     * Identfier for the access code
     */
    uint32_t identifier;

    /**
     * The access code
     */
    char* accessCode;

    /**
     * Bitmask indicating the restrictions on the access code
     */
    uint32_t flags;

    /**
     * Identifier enumeration callback
     */
    HAPAccessCodeIdentifierCallback enumerateCallback;

    /**
     * Identifier enumeration callback context
     */
    void* enumerateContext;

    /**
     * Input identifier is found.
     */
    bool found : 1;

    /**
     * Identifier is duplicate
     */
    bool duplicate : 1;

    /**
     * Storage is full and operation cannot complete.
     */
    bool outOfResources : 1;

    /**
     * Input parameter too short
     */
    bool parameterTooShort : 1;

    /**
     * Input parameter too long
     */
    bool parameterTooLong : 1;

    /**
     * Unacceptable access code character set
     */
    bool invalidCharacterSet : 1;
} HAPAccessCodeOperation;

/**
 * Access Code Operation callback function
 *
 * @param[inout] op     operation and result.<br>
 *                      Note that pointers in the result must be valid till another operation
 *                      callback is made.
 * @param        ctx    context
 *
 * @return kHAPError_None when operation could result into a response whether successful or not.<br>
 *         Other error code when an unexpected error occurred and a normal response cannot be generated.
 */
typedef HAPError (*HAPAccessCodeOperationCallback)(HAPAccessCodeOperation* op, void* _Nullable ctx);

/**
 * Access Code response storage.
 *
 * Accessory supporting Access Code service must add response storage
 * with sessions as many as maximum number of queued operation responses
 * prior to a notification
 */
typedef struct {
    /**
     * Response TLV data buffer. Memory must be allocated and must remain valid while the accessory server is
     * initialized.
     *
     * The size of the buffer would depend on maximum number of the responses to carry,
     * maximum number of Access Codes and maximum length of Access Codes, etc.
     */
    void* bytes;

    /**
     * Size of the buffer.
     */
    size_t maxBytes;
} HAPAccessCodeResponseStorage;
HAP_NONNULL_SUPPORT(HAPAccessCodeResponseStorage)
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
/**
 * NFC Access response storage.
 *
 * Accessory supporting NFC Access service must add response storage
 * with sessions as many as maximum number of queued operation responses
 * prior to a notification
 */
typedef struct {
    /**
     * Response TLV data buffer. Memory must be allocated and must remain valid while the accessory server is
     * initialized.
     *
     * The size of the buffer depends on maximum number of NFC Access keys and type of keys stored.
     */
    void* bytes;

    /**
     * Size of the buffer in bytes.
     */
    size_t maxBytes;
} HAPNfcAccessResponseStorage;
HAP_NONNULL_SUPPORT(HAPNfcAccessResponseStorage)
#endif

/**
 * HAP over IP (Ethernet / Wi-Fi) accessory server transport.
 */
/**@{*/
typedef struct HAPIPAccessoryServerTransport HAPIPAccessoryServerTransport;
extern const HAPIPAccessoryServerTransport kHAPAccessoryServerTransport_IP;
HAP_NONNULL_SUPPORT(HAPIPAccessoryServerTransport)
/**@}*/

/**
 * Thread session.
 */
struct _HAPThreadSession;
typedef struct _HAPThreadSession HAPThreadSession;
HAP_NONNULL_SUPPORT(HAPThreadSession)

/**
 * Minimum number of sessions in a HAPThreadAccessoryServerStorage.
 */
#define kHAPThreadAccessoryServerStorage_MinSessions ((size_t) 8)

/**
 * Function prototype to get the delay till the next Thread reattach attempt.
 *
 * @param context        context
 * @param reattachCount  number of times reattach was already attempted since the last successful join.
 *
 * @return duration till next Thread reattach attempt in HAPTime unit.
 */
typedef HAPTime (*HAPThreadGetReattachDelay)(void* _Nullable context, uint32_t reattachCount);

/**
 * Thread accessory server storage.
 *
 * - For accessories that support Thread, one of these structures needs to be allocated
 *   and provided as part of the initialization options in HAPAccessoryServerCreate.
 *
 * - The provided memory (including the HAPThreadAccessoryServerStorage structure) must remain valid
 *   while the accessory server is initialized.
 */
typedef struct {
    /**
     * Thread sessions.
     *
     * - One session must be provided per concurrently supported Thread peer.
     *
     * - At least kHAPThreadAccessoryServerStorage_MinSessions elements are required for Thread accessories.
     */
    HAPThreadSession* sessions;

    /**
     * Number of sessions.
     */
    size_t numSessions;

    /** Thread Network joining parameters */
    struct {
        /** network parameters are set */
        bool parametersAreSet : 1;
        /** Thread joiner is requested instead of explicit parameters */
        bool joinerRequested : 1;
        /** network forming is allowed */
        bool formingAllowed : 1;
        /** joining is in progress */
        bool inProgress : 1;
        /** joining was successful */
        bool joinSuccessful : 1;
        /** reachability result */
        bool reachable : 1;
        /** joiner detected a security error */
        bool securityError : 1;
        /** use static commissioning */
        bool staticCommissioning : 1;
        /** whether to try to reattach after Thread joining fails */
        bool shouldReattach : 1;
        /** attach timeout duration */
        uint32_t attachTimeout;
        /** number of times reattach was tried */
        uint32_t reattachCount;
        /** network parameters */
        HAPPlatformThreadNetworkParameters parameters;
        /** wake-lock for border router detection */
        HAPPlatformThreadWakeLock borderRouterDetectionWakeLock;
        /** joiner timer */
        HAPPlatformTimerRef joinerTimer;
        /** reattach timer */
        HAPPlatformTimerRef reattachTimer;
        /** timer to keep track of max time to wait to find a border router in the thread network */
        HAPPlatformTimerRef borderRouterDetectTimer;
        /** system time when attach timeout expires */
        HAPTime attachExpireTime;
    } networkJoinState;

    /** function to get the delay till the next Thread reattach attempt */
    HAPThreadGetReattachDelay _Nullable getNextReattachDelay;

    /** context to call the getNextReattachDelay function with */
    void* _Nullable getNextReattachDelayContext;
} HAPThreadAccessoryServerStorage;
HAP_NONNULL_SUPPORT(HAPThreadAccessoryServerStorage)

/**
 * HAP over Thread accessory server transport.
 */
/**@{*/
typedef struct HAPThreadAccessoryServerTransport HAPThreadAccessoryServerTransport;
extern const HAPThreadAccessoryServerTransport kHAPAccessoryServerTransport_Thread;
HAP_NONNULL_SUPPORT(HAPThreadAccessoryServerTransport)
/**@}*/

/**
 * Element of the BLE GATT table.
 *
 * - For accessories that support Bluetooth LE, at least one of these elements must be allocated per HomeKit
 *   characteristic and service, and provided as part of a HAPBLEAccessoryServerStorage structure.
 */
struct _HAPBLEGATTTableElement;
typedef struct _HAPBLEGATTTableElement HAPBLEGATTTableElement;

/**
 * Minimum number of BLE session cache elements in a HAPBLEAccessoryServerStorage.
 */
#define kHAPBLESessionCache_MinElements ((size_t) 8)

/**
 * Element of the BLE Pair Resume session cache.
 *
 * - For accessories that support Bluetooth LE, at least kHAPBLESessionCache_MinElements
 *   of these elements must be allocated and provided as part of a HAPBLEAccessoryServerStorage structure.
 */
struct _HAPPairingBLESessionCacheEntry;
typedef struct _HAPPairingBLESessionCacheEntry HAPPairingBLESessionCacheEntry;

/**
 * HAP-BLE procedure.
 *
 * - For accessories that support Bluetooth LE, at least one of these procedures must be allocated
 *   and provided as part of a HAPBLEAccessoryServerStorage structure.
 */
struct _HAPBLEProcedure;
typedef struct _HAPBLEProcedure HAPBLEProcedure;

/**
 * BLE accessory server storage.
 *
 * - For accessories that support Bluetooth LE, one of these structures needs to be allocated
 *   and provided as part of the initialization options in HAPAccessoryServerCreate.
 *
 * - The provided memory (including the HAPBLEAccessoryServerStorage structure) must remain valid
 *   while the accessory server is initialized.
 */
typedef struct {
    /**
     * BLE GATT table elements.
     *
     * - At least one of these elements is required per HomeKit characteristic and service
     *   and must remain valid while the accessory server is initialized.
     */
    HAPBLEGATTTableElement* gattTableElements;

    /**
     * Number of BLE GATT table elements.
     */
    size_t numGATTTableElements;

    /**
     * BLE Pair Resume session cache. Storage must remain valid.
     *
     * - Controllers may use the cache to speed up re-connections to the accessory by allowing them
     *   to resume previously established sessions ("Pair Resume").
     *
     * - The cache size determines how many different controllers can take advantage of this feature
     *   before they have to do a re-connection with regular speed again.
     *
     * - At least kHAPBLESessionCache_MinElements elements are required
     *   and must remain valid while the accessory server is initialized.
     */
    HAPPairingBLESessionCacheEntry* sessionCacheElements;

    /**
     * Number of BLE session cache elements.
     */
    size_t numSessionCacheElements;

    /**
     * BLE session storage. Storage must remain valid.
     */
    HAPSession* session;

    /**
     * HAP-BLE procedures. Storage must remain valid.
     */
    HAPBLEProcedure* procedures;

    /**
     * Number of HAP-BLE procedures.
     */
    size_t numProcedures;

    /**
     * Buffer that the HAP-BLE procedures may use.
     *
     * - This must be large enough to fit the largest characteristic value.
     */
    struct {
        /**
         * HAP-BLE procedure buffer.
         */
        void* bytes;

        /**
         * Size of HAP-BLE procedure buffer.
         */
        size_t numBytes;
    } procedureBuffer;

    /**
     * Buffer that holds the value of the next event notification to be sent.
     *
     * - This must be large enough to fit the largest characteristic value that supports event notifications.
     */
    struct {
        /** Buffer. */
        void* _Nullable bytes;

        /** Size of buffer. */
        size_t numBytes;
    } eventBuffer;
} HAPBLEAccessoryServerStorage;
HAP_NONNULL_SUPPORT(HAPBLEAccessoryServerStorage)

/**
 * Minimum supported advertising interval for Bluetooth LE.
 */
#define kHAPBLEAdvertisingInterval_Minimum (HAPBLEAdvertisingIntervalCreateFromMilliseconds(160))

/**
 * Maximum supported advertising interval for Bluetooth LE.
 */
#define kHAPBLEAdvertisingInterval_Maximum (HAPBLEAdvertisingIntervalCreateFromMilliseconds(2500))

/**
 * Minimum duration of broadcast notifications and disconnected notifications in advertising interval of Bluetooth LE
 * (0.625ms).
 */
#define kHAPBLENotification_MinDuration (HAPBLEAdvertisingIntervalCreateFromMilliseconds(3000))

/**
 * HAP over Bluetooth LE accessory server transport.
 */
/**@{*/
typedef struct HAPBLEAccessoryServerTransport HAPBLEAccessoryServerTransport;
extern const HAPBLEAccessoryServerTransport kHAPAccessoryServerTransport_BLE;
HAP_NONNULL_SUPPORT(HAPBLEAccessoryServerTransport)
/**@}*/

/**
 * Accessory server initialization options.
 */
typedef struct {
    /**
     * Maximum number of allowed pairings.
     *
     * - Must be at least kHAPPairingStorage_MinElements.
     */
    HAPPlatformKeyValueStoreKey maxPairings;

    /**
     * Session key valid duration in milliseconds, after which session key expires, or
     * zero if the session key should be valid forever.
     */
    HAPTime sessionKeyExpiry;

    /**
     * Session storage buffer that may be used for request handling.
     *
     * - The optimal size may vary depending on the accessory's HomeKit attribute database, the number of sessions
     *   and the requests to be processed. Log messages are emitted to provide guidance for tuning of the buffer size.
     *
     * - Note: Buffer usage may vary across controller versions, so it is advised to not set the size too tightly.
     */
    struct {
        void* _Nullable bytes; /**< Buffer. */
        size_t numBytes;       /**< Length of buffer. */
    } sessionStorage;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP) || HAP_FEATURE_ENABLED(HAP_FEATURE_HDS_TRANSPORT_OVER_HAP)
    /**
     * Data Stream options.
     *
     * To enable Data Stream, initialize the `dataStream` to point to an array of `numDataStreams` object.
     *
     * Data Stream supports two transports, and only one is allowed:
     *  - set up only this struct in order to get HDS over the HAP transport.
     *  - set `ip.dataStream.tcpStreamManager` as well in order to get HDS over the TCP transport.
     */
    struct {
        /**
         * Storage for HomeKit Data Streams. Must remain valid.
         */
        HAPDataStreamRef* _Nullable dataStreams;

        /**
         * Number of HomeKit Data Streams.
         */
        size_t numDataStreams;
    } dataStream;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    /**
     * IP specific initialization options.
     *
     * - Accessories which use IP as a transport to communicate with iOS or other devices must declare their network
     *   services access information (Internet and local network) in order to establish WAN and/or LAN firewall rules
     *   in a HomeKit-enabled Wi-Fi router. These network declarations must be submitted through the MFi Portal
     *   when prompted.
     *
     * @see HomeKit Accessory Protocol Specification R17
     *      Section 3.7 Network Declarations
     */
    struct {
        /**
         * Transport.
         *
         * - If the accessory supports HAP over IP (Ethernet / Wi-Fi), must refer to kHAPAccessoryServerTransport_IP.
         *
         * - If this is set to NULL, the other IP specific initialization options are ignored.
         */
        const HAPIPAccessoryServerTransport* _Nullable transport;

        /**
         * IP accessory server storage. Storage must remain valid.
         */
        HAPIPAccessoryServerStorage* _Nullable accessoryServerStorage;

        /**
         * HomeKit Data Stream specific initialization options.
         */
        struct {
            /**
             * TCP stream manager.
             *
             * - This is only necessary if the accessory supports HomeKit Data Stream.
             *
             * - Needs to be a separate instance other than the TCP stream manager part of HAPPlatform.
             *
             * - The TCP port range of this instance must be >= 32768.
             */
            HAPPlatformTCPStreamManagerRef _Nullable tcpStreamManager;
        } dataStream;

        /**
         * Wi-Fi Accessory Configuration (WAC) specific initialization options.
         */
        struct {
            /**
             * Accessory supports Wi-Fi Accessory Configuration (WAC) for configuring Wi-Fi credentials.
             *
             * - Wi-Fi accessories must support WAC as a method for configuring Wi-Fi credentials.
             *
             * - Requires the platform to support authentication.
             */
            bool available : 1;
        } wac;

        /**
         * IP Camera specific initialization options.
         */
        struct {
            /**
             * IP Camera streaming sessions. Storage must remain valid.
             */
            HAPCameraStreamingSessionStorage* _Nullable streamingSessionStorage;
#if (HAP_TESTING == 1)
            /**
             * Directory where Media files are stored.
             */
            const char* _Nullable mediaSourcePath;
#endif
        } camera;
    } ip;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    /**
     * BLE specific initialization options.
     */
    struct {
        /**
         * Transport.
         *
         * - If the accessory supports HAP over Bluetooth LE, must refer to kHAPAccessoryServerTransport_BLE.
         *
         * - If this is set to NULL, the other BLE specific initialization options are ignored.
         */
        const HAPBLEAccessoryServerTransport* _Nullable transport;

        /**
         * BLE accessory server storage. Storage must remain valid.
         */
        HAPBLEAccessoryServerStorage* _Nullable accessoryServerStorage;

        /**
         * Preferred Regular Advertising Interval in ms.
         *
         * Accessories must choose the regular advertising intervals in the range of kHAPBLEAdvertisingInterval_Minimum
         * ms to kHAPBLEAdvertisingInterval_Maximum ms, depending on the accessory category and its power consumption
         * characteristics.firmwareUpdate
         *
         * Mains powered accessories or accessories with larger battery capacity should use a shorter interval between
         * 160 ms to 800 ms for its regular advertisements. Battery powered accessories that do not have any
         * controllable Apple defined characteristics (such as temperature sensors, door sensors etc) are allowed to
         * use a larger regular advertising interval between 1250 ms and 2500 ms for enhanced battery life.
         *
         * Preferred advertising intervals:
         * - 211.25 ms
         * - 318.75 ms
         * - 417.5 ms
         * - 546.25 ms
         * - 760 ms
         * - 852.5 ms
         * - 1022.5 ms
         * - 1285 ms
         *
         * Note: Longer advertising intervals usually result in longer discovery and connection times.
         *
         * Use HAPBLEAdvertisingIntervalCreateFromMilliseconds to convert an advertising interval in milliseconds
         * to an advertising interval for Bluetooth LE.
         *
         * @see HomeKit Accessory Protocol Specification R17
         *      Section 7.4.1.4 Advertising Interval
         *
         * @see Accessory Design Guidelines for Apple Devices R7
         *      Section 11.5 Advertising Interval
         */
        HAPBLEAdvertisingInterval preferredAdvertisingInterval;

        /**
         * Preferred duration of events in advertising interval of Bluetooth LE (0.625ms).
         *
         * Must be at least kHAPBLENotification_MinDuration.
         */
        HAPBLEAdvertisingInterval preferredNotificationDuration;
    } ble;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    /**
     * Thread specific initialization options.
     */
    struct {
        /**
         * Transport.
         *
         * - If the accessory supports HAP over Thread, must refer to kHAPAccessoryServerTransport_Thread.
         *
         * - If this is set to NULL, the other Thread specific initialization options are ignored.
         */
        const HAPThreadAccessoryServerTransport* _Nullable transport;

        /**
         * Thread accessory server storage. Storage must remain valid.
         */
        HAPThreadAccessoryServerStorage* _Nullable accessoryServerStorage;

        struct {
            /**
             * Thread device type
             *
             * The value must be one of the following:
             * @ref kHAPPlatformThreadDeviceCapabilities_BR,
             * @ref kHAPPlatformThreadDeviceCapabilities_REED,
             * @ref kHAPPlatformThreadDeviceCapabilities_FED,
             * @ref kHAPPlatformThreadDeviceCapabilities_MED or
             * @ref kHAPPlatformThreadDeviceCapabilities_SED.
             */
            HAPPlatformThreadDeviceCapabilities deviceType;

            /** Thread child timeout in seconds */
            uint32_t childTimeout;

            /**
             * Power leve in db
             */
            uint8_t txPowerdbm;
            uint32_t sleepInterval;
        } deviceParameters;

        /**
         * Custom function to compute the delay till the next Thread reattach attempt.
         * Set to NULL in order to use the default logic.
         */
        HAPThreadGetReattachDelay _Nullable getNextReattachDelay;

        /** context to call the getNextReattachDelay function with */
        void* _Nullable getNextReattachDelayContext;

        bool suppressUnpairedThreadAdvertising;
    } thread;
#endif

#if (HAP_TESTING == 1)
    /**
     * Firmware update specific initialization options.
     */
    struct {
        bool persistStaging;
    } firmwareUpdate;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_ACCESS_CODE)
    /**
     * Access Code service specific initialization options.
     */
    struct {
        /**
         * Response storage
         */
        HAPAccessCodeResponseStorage* responseStorage;

        /**
         * Access Code operation callback
         */
        HAPAccessCodeOperationCallback handleOperation;

        /**
         * Access Code operation callback context
         */
        void* operationCtx;
    } accessCode;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_NFC_ACCESS)
    /**
     * NFC Access service specific initialization options.
     */
    struct {
        /**
         * Response storage
         */
        HAPNfcAccessResponseStorage* responseStorage;
    } nfcAccess;
#endif
} HAPAccessoryServerOptions;

/**
 * HomeKit platform structure.
 *
 * The following platform modules must be implemented additionally:
 * - HAPPlatformAbort
 * - HAPPlatformLog
 * - HAPPlatformRandomNumber
 * - HAPPlatformClock
 * - HAPPlatformTimer
 * - HAPPlatformRunLoop
 */
typedef struct {
    /**
     * Key-Value store.
     */
    HAPPlatformKeyValueStoreRef keyValueStore;

    /**
     * Accessory setup manager.
     */
    HAPPlatformAccessorySetupRef accessorySetup;

    /**
     * Accessory setup display.
     *
     * - This platform module is only necessary if the accessory supports displaying accessory setup information.
     */
    HAPPlatformAccessorySetupDisplayRef _Nullable setupDisplay;

    /**
     * Accessory setup programmable NFC tag.
     *
     * - This platform module is only necessary if the accessory supports a programmable NFC tag for accessory setup.
     */
    HAPPlatformAccessorySetupNFCRef _Nullable setupNFC;

#if HAP_FEATURE_ENABLED(HAP_FEATURE_IP)
    /**
     * These platform modules are only necessary if the accessory supports HAP over IP (Ethernet / Wi-Fi).
     */
    struct {
        /**
         * TCP stream manager.
         */
        HAPPlatformTCPStreamManagerRef _Nullable tcpStreamManager;

        /**
         * Service discovery. Used by IP and Thread transports
         */
        HAPPlatformServiceDiscoveryRef _Nullable serviceDiscovery;

        /**
         * These platform modules are only necessary if the accessory supports Wi-Fi.
         */
        struct {
            /**
             * Software Access Point manager.
             */
            HAPPlatformSoftwareAccessPointRef _Nullable softwareAccessPoint;

            /**
             * Wi-Fi manager.
             */
            HAPPlatformWiFiManagerRef _Nullable wiFiManager;
        } wiFi;

        /**
         * IP Camera provider.
         *
         * - This platform module is only necessary if the accessory is an IP Camera.
         */
        HAPPlatformCameraRef _Nullable camera;

        /**
         * Wi-Fi Router.
         *
         * - This platform module is only necessary if the accessory is a Wi-Fi router.
         */
        HAPPlatformWiFiRouterRef _Nullable wiFiRouter;
    } ip;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    /**
     * These platform modules are only necessary if the accessory supports HAP over Bluetooth LE.
     */
    struct {
        /**
         * BLE peripheral manager.
         */
        HAPPlatformBLEPeripheralManagerRef _Nullable blePeripheralManager;
    } ble;
#endif

#if HAP_FEATURE_ENABLED(HAP_FEATURE_THREAD)
    /**
     * These platform modules are only necessary if the accessory supports HAP over Thread.
     */
    struct {
        /**
         * CoAP manager.
         */
        HAPPlatformThreadCoAPManagerRef _Nullable coapManager;

        /**
         * Service discovery. Used by IP and Thread transports
         */
        HAPPlatformServiceDiscoveryRef _Nullable serviceDiscovery;
    } thread;
#endif

    /**
     * These platform modules are only necessary for production accessories or
     * if the accessory supports Wi-Fi Accessory Configuration (WAC) for configuring Wi-Fi credentials.
     *
     * - Only one authentication method needs to be implemented.
     */
    struct {
        /**
         * Apple Authentication Coprocessor provider.
         *
         * - This platform module is only necessary if an Apple Authentication Coprocessor is connected.
         */
        HAPPlatformMFiHWAuthRef _Nullable mfiHWAuth;

        /**
         * Software Token provider.
         *
         * - This platform module is only necessary if Software Authentication is supported.
         */
        HAPPlatformMFiTokenAuthRef _Nullable mfiTokenAuth;
    } authentication;
} HAPPlatform;

/**
 * Pairing state change event.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPairingStateChange) {
    /* Pairing state of first admin controller changed to paired. */
    kHAPPairingStateChange_Paired,

    /* Pairing state of last admin controller changed to unpaired. */
    kHAPPairingStateChange_Unpaired,
} HAP_ENUM_END(uint8_t, HAPPairingStateChange);

/**
 * Controller pairing state change event.
 */
HAP_ENUM_BEGIN(uint8_t, HAPControllerPairingStateChange) {
    /* Pairing state of any controller (first admin or additional) changed to paired. */
    kHAPControllerPairingStateChange_Paired,

    /* Pairing state of any controller changed to unpaired. */
    kHAPControllerPairingStateChange_Unpaired,
} HAP_ENUM_END(uint8_t, HAPControllerPairingStateChange);

/**
 * Pairing identifier of a paired controller.
 */
typedef struct {
    /**
     * Buffer containing pairing identifier.
     */
    uint8_t bytes[36];

    /**
     * Number of used bytes in buffer.
     */
    size_t numBytes;
} HAPControllerPairingIdentifier;

/**
 * Public key of a paired controller.
 */
typedef struct {
    /**
     * Public key.
     */
    uint8_t bytes[32];
} HAPControllerPublicKey;
HAP_STATIC_ASSERT(sizeof(HAPControllerPublicKey) == 32, HAPControllerPublicKey);

/**
 * Accessory server callbacks.
 *
 * - Callbacks must not block.
 */
typedef struct {
    /**
     * Invoked when the accessory server state changes.
     *
     * - Updated state may be retrieved through the HAPAccessoryServerGetState, HAPAccessoryServerIsPaired and
     *   HAPAccessoryServerIsInWACMode methods.
     *
     * - The callback must not block.
     *
     * @param      server               Accessory server.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleUpdatedState)(HAPAccessoryServer* server, void* _Nullable context);

    /**
     * The callback used when a HomeKit Session is accepted.
     *
     * @param      server               Accessory server.
     * @param      session              The newly accepted session.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleSessionAccept)(HAPAccessoryServer* server, HAPSession* session, void* _Nullable context);

    /**
     * The callback used when a HomeKit Session is invalidated.
     *
     * - /!\ WARNING: The HomeKit Session must no longer be used after this callback returns.
     *
     * @param      server               Accessory server.
     * @param      session              The session being invalidated.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handleSessionInvalidate)(HAPAccessoryServer* server, HAPSession* session, void* _Nullable context);

    /**
     * Invoked when the accessory pairing state changes.
     *
     * @param      server               Accessory server.
     * @param      state                The new pairing state.
     * @param      context              The context parameter given to the HAPAccessoryServerCreate function.
     */
    void (*handlePairingStateChange)(HAPAccessoryServer* server, HAPPairingStateChange state, void* _Nullable context);

    /**
     * Invoked when an admin or additional controller has been paired/unpaired.
     *
     * @param      server               Accessory server.
     * @param      state                The new controller pairing state.
     * @param      pairingIdentifier    Pairing identifier.
     * @param      publicKey            Controller public key.
     * @param      context              The context parameter given to the application.
     */
    void (*_Nullable handleControllerPairingStateChange)(
            HAPAccessoryServer* server,
            HAPControllerPairingStateChange state,
            const HAPControllerPairingIdentifier* pairingIdentifier,
            const HAPControllerPublicKey* publicKey,
            void* _Nullable context);
} HAPAccessoryServerCallbacks;

/**
 * Creates a new HomeKit accessory server.
 *
 * - Callbacks are always invoked synchronously. They must not block.
 *   This also applies to characteristic value read / write callbacks.
 *
 * @param[out] server               An uninitialized accessory server.
 * @param      options              Initialization options for the accessory server.
 * @param      platform             Initialized HomeKit platform structure.
 * @param      callbacks            Callbacks to receive server events. Callbacks must not block.
 * @param      context              Client context pointer. Will be passed to callbacks.
 */
void HAPAccessoryServerCreate(
        HAPAccessoryServer* server,
        const HAPAccessoryServerOptions* options,
        const HAPPlatform* platform,
        const HAPAccessoryServerCallbacks* callbacks,
        void* _Nullable context);

/**
 * Releases resources associated with an initialized HomeKit accessory server.
 *
 * - If the accessory server is currently running, it will be ungracefully stopped by this method.
 *   Consider stopping the accessory server first and waiting for proper shutdown to occur before releasing resources.
 *
 * - IMPORTANT: Do not use this method on accessory server structures that are not initialized!
 *
 * @param      server               An initialized accessory server.
 */
void HAPAccessoryServerRelease(HAPAccessoryServer* server);

/**
 * Accessory server state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPAccessoryServerState) { /** Server is initialized but not running. */
                                                   kHAPAccessoryServerState_Idle,

                                                   /** Server is running. */
                                                   kHAPAccessoryServerState_Running,

                                                   /** Server is shutting down. */
                                                   kHAPAccessoryServerState_Stopping
} HAP_ENUM_END(uint8_t, HAPAccessoryServerState);

/**
 * Gets the state of an initialized HomeKit accessory server.
 *
 * @param      server               An initialized accessory server.
 *
 * @return Accessory server state.
 */
HAP_RESULT_USE_CHECK
HAPAccessoryServerState HAPAccessoryServerGetState(HAPAccessoryServer* server);

/**
 * Returns whether the HomeKit accessory server is paired with any controllers.
 *
 * @param      server               An initialized accessory server.
 *
 * @return Whether the accessory server is paired with any controllers.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessoryServerIsPaired(HAPAccessoryServer* server);

/**
 * Returns whether the HomeKit accessory server is in WAC mode.
 *
 * @param      server               An initialized accessory server.
 *
 * @return Whether the accessory server is in WAC mode.
 */
HAP_RESULT_USE_CHECK
bool HAPAccessoryServerIsInWACMode(HAPAccessoryServer* server);

/**defgroup OwnershipProof Ownership proof.
 *
 * When an accessory is paired through a non-HomeKit mechanism, ownership proof can be used
 * to restrict HomeKit pairing to users who have the necessary privileges in said non-HomeKit system.
 * Ownership proof requires an accessory app that can obtain a token from the accessory which proves
 * that the user is authorized to perform HomeKit pairing.
 *
 * Example flow:
 * 1) User requests HomeKit pairing in accessory app.
 * 2) Accessory app communicates with the accessory and verifies that the user is authorized for HomeKit pairing.
 * 3) Accessory generates ownership proof token using HAPAccessoryServerGenerateOwnershipProofToken.
 * 4) Accessory app obtains the generated ownership proof token from the accessory.
 * 5) Accessory app provides the ownership proof token using -[HMAccessorySetupPayload initWithURL:ownershipToken:].
 * 6) Accessory app requests HomeKit pairing using -[HMHome addAndSetupAccessoriesWithPayload:completionHandler:].
 * 7) During the HomeKit pairing process, the app-provided ownership proof token is sent to the accessory.
 * 8) Accessory verifies that the received ownership proof token matches the one generated earlier.
 * 9) After HomeKit pairing completes, user gains HomeKit admin privileges.
 */
/**@{*/

/**
 * Updates whether an ownership proof token is required for accessory setup.
 *
 * - Ownership proof tokens are used when an accessory server supports out-of-band pairing mechanisms.
 *   A controller may only complete accessory setup successfully if a valid ownership proof token is presented.
 *   The ownership proof token has to be transferred to authorized controllers using an out-of-band mechanism.
 *
 * - This function may be called before the accessory server is started.
 *   This way, an ownership proof token is required for accessory setup as soon as the accessory server is started.
 *
 * - Whenever this function is called, the generated ownership proof token is invalidated if applicable.
 *
 * @param      server               An initialized accessory server.
 * @param      tokenRequired        Whether an ownership proof token should be required for accessory setup.
 */
void HAPAccessoryServerSetOwnershipProofTokenRequired(HAPAccessoryServer* server, bool tokenRequired);

/**
 * Ownership proof token for accessory setup.
 */
typedef struct {
    uint8_t bytes[16]; /**< Value. */
} HAPAccessorySetupOwnershipProofToken;

/**
 * Generates and returns a valid ownership proof token for accessory setup.
 *
 * - A controller may only complete accessory setup successfully if a valid ownership proof token is presented.
 *   The ownership proof token has to be transferred to authorized controllers using an out-of-band mechanism.
 *
 * - The ownership proof token is only valid for a limited time and for one pairing attempt.
 *   Accessory setup fails if an expired ownership proof token is presented.
 *
 * - Only the most recently generated ownership proof token is valid.
 *
 * @param      server               An initialized accessory server.
 * @param[out] ownershipToken       Ownership proof token for accessory setup.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If ownership proof token is not required for accessory setup,
 *                                  or if the accessory server is already paired.
 */
HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerGenerateOwnershipProofToken(
        HAPAccessoryServer* server,
        HAPAccessorySetupOwnershipProofToken* ownershipToken);

/**@}*/

/**
 * Starts the accessory server.
 *
 * - To start a bridge, use HAPAccessoryServerStartBridge instead.
 *
 * - The server state can be observed using the handleUpdatedState callback. The server never stops on its own.
 *
 * - The server will enter WAC mode automatically if WAC is supported by the platform and when it is started in a
 *   non-configured state (see functions HAPAccessoryServerIsInWACMode and HAPAccessoryServerEnterWACMode).
 *
 * - Note that the advertised accessory's attribute database including its services and characteristics
 *   usually only changes after a firmware update.
 *   - When a service or characteristic is removed, associated automations configured via the Apple Home app
 *     may be automatically removed as well. If the service or characteristic is later re-introduced,
 *     these removed automations will not be restored automatically.
 *   - When using HAP over Bluetooth LE the Bluetooth SIG defined "Service Changed" GATT characteristic is typically
 *     omitted for performance reasons, leading to controllers caching the attribute database on the Bluetooth layer.
 *     on a firmware update, a new Bluetooth address is generated to ensure invalidation of this cache.
 *     See HomeKit Accessory Protocol Specification R17
 *     Section 7.4.8 Firmware Update Requirements
 *   - When using HAP over IP (Ethernet / Wi-Fi), adding or removing services and characteristics is technically
 *     possible. However, for IP Cameras, that is only allowed via a firmware update to the accessory.
 *     See HomeKit Accessory Protocol Specification R17
 *     Section 12.4.3 Multiple Camera RTP Stream Management
 *   - Test case TCH060 verifies that the advertised attribute database remains the same across power cycles.
 *     See HomeKit Certification Test Cases R8.3
 *   - If it is necessary to add or remove services and characteristics without a firmware update
 *     please use the HAPAccessoryServerStartBridge API instead (passing an empty array as bridgedAccessories),
 *     and set configurationChanged to true.
 *
 * @param      server               An initialized accessory server that is not running.
 * @param      accessory            Accessory to serve. Must remain valid while started.
 */
void HAPAccessoryServerStart(HAPAccessoryServer* server, const HAPAccessory* accessory);

/**
 * Maximum number of supported bridged accessories not including the bridge itself.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 2.5.3.2 Bridges
 */
#define kHAPAccessoryServerMaxBridgedAccessories ((size_t) 149)

/**
 * Starts the accessory server for a bridge accessory. A bridge is a special type of HAP accessory server that
 * bridges HomeKit Accessory Protocol and different RF/transport protocols, such as ZigBee or Z-Wave.
 * A bridge must ensure that the instance ID assigned to the HAP accessory objects exposed on behalf of its
 * connected bridged endpoints do not change for the lifetime of the server/client pairing.
 *
 * - Only supported for HAP over IP (Ethernet / Wi-Fi).
 *
 * - A bridge accessory must not bridge more than kHAPAccessoryServerMaxBridgedAccessories accessories.
 *
 * - To change the bridged accessories (e.g., after firmware update or after modified bridge configuration),
 *   stop the server, then apply changes to the @p bridgedAccessories array, then start the server again.
 *
 * - The server state can be observed using the handleUpdatedState callback. The server never stops on its own.
 *
 * - The server will enter WAC mode automatically if WAC is supported by the platform and when it is started in a
 *   non-configured state (see functions HAPAccessoryServerIsInWACMode and HAPAccessoryServerEnterWACMode).
 *
 * @param      server               An initialized accessory server that is not running.
 * @param      bridgeAccessory      Bridge accessory to serve. Must remain valid while started.
 * @param      bridgedAccessories   Array of bridged accessories. NULL-terminated. Must remain valid while started.
 * @param      configurationChanged Whether or not the bridge configuration changed since the last start.
 *                                  This includes adding / removing accessories or updating FW of a bridged accessory.
 */
void HAPAccessoryServerStartBridge(
        HAPAccessoryServer* server,
        const HAPAccessory* bridgeAccessory,
        const HAPAccessory* _Nullable const* _Nullable bridgedAccessories,
        bool configurationChanged);

/**
 * Starts the accessory server for a camera bridge accessory. A bridge is a special type of HAP accessory server that
 * bridges HomeKit Accessory Protocol and different RF/transport protocols, such as ZigBee or Z-Wave.
 * A bridge must ensure that the instance ID assigned to the HAP accessory objects exposed on behalf of its
 * connected bridged endpoints do not change for the lifetime of the server/client pairing.
 *
 * - Only supported for HAP over IP (Ethernet / Wi-Fi).
 *
 * - A bridge accessory must not bridge more than kHAPAccessoryServerMaxBridgedAccessories accessories.
 *
 * - To change the bridged accessories (e.g., after firmware update or after modified bridge configuration),
 *   stop the server, then apply changes to the @p bridgedAccessories array, then start the server again.
 *
 * - The bridgedCameras array must have the same size as the bridgedAccessories.
 *   If an entry corresponds to a camera accessory, it must point to the camera, otherwise it must be NULL.
 *
 * - The server state can be observed using the handleUpdatedState callback. The server never stops on its own.
 *
 * - The server will enter WAC mode automatically if WAC is supported by the platform and when it is started in a
 *   non-configured state (see functions HAPAccessoryServerIsInWACMode and HAPAccessoryServerEnterWACMode).
 *
 * @param      server               An initialized accessory server that is not running.
 * @param      bridgeAccessory      Bridge accessory to serve. Must remain valid while started.
 * @param      bridgedAccessories   Array of bridged accessories. NULL-terminated. Must remain valid while started.
 * @param      bridgedCameras       Array of bridged cameras. Same size as bridgedAccessories.
 * @param      configurationChanged Whether or not the bridge configuration changed since the last start.
 *                                  This includes adding / removing accessories or updating FW of a bridged accessory.
 */
void HAPAccessoryServerStartCameraBridge(
        HAPAccessoryServer* server,
        const HAPAccessory* bridgeAccessory,
        const HAPAccessory* _Nullable const* _Nullable bridgedAccessories,
        const HAPPlatformCameraRef _Nullable* _Nullable bridgedCameras,
        bool configurationChanged);

/**
 * Schedules a main thread task to stop the accessory server.
 *
 * - When the accessory server is fully stopped, the handleUpdatedState callback will be invoked.
 *   The server can only be started again after that callback has been invoked.
 *
 * @param      server               An initialized accessory server.
 */
void HAPAccessoryServerStop(HAPAccessoryServer* server);

/**
 * Stops the accessory server immediately, instead of scheduling.
 * This method makes assumptions about HAP state, and is not always safe to call.
 * Use HAPAccessoryServerStop() instead whenever possible.
 *
 * - When the accessory server is fully stopped, the handleUpdatedState callback will be invoked.
 *   The server can only be started again after that callback has been invoked.
 *
 * @param      server               An initialized accessory server.
 */
void HAPAccessoryServerForceStop(HAPAccessoryServer* server);

/**
 * Keeps the BLE of the accessory server.
 *
 * Application can use this function to keep the BLE on while using the server which may turn on
 * and turn off BLE, such as when Thread is concurrently supported.
 * In such a case, BLE may get turned on by calling this function while BLE is turned off.
 *
 * Note that demanding BLE to be kept on does not work in such a case where the platform does
 * not not allow BLE to be operating agnostic to other potentially conflicting radios,
 * for example, in case the concurrent BLE and Thread is not supported.
 *
 * @param      server               An initialized accessory server.
 * @param      on                   Set to true to keep the BLE on for the application use.
 *                                  Set to false, if BLE can be turned off based on server state.
 */
void HAPAccessoryServerKeepBleOn(HAPAccessoryServer* server, bool on);

/**
 * Refreshes the setup payload that is presented for pairing on applicable displays and programmable NFC tags.
 *
 * - The setup payload is automatically refreshed periodically if no pairing attempt is in progress.
 *   This function may be used to manually refresh the setup payload.
 *
 * - If a pairing attempt is in progress, the setup payload may not be refreshed and this function has no effect.
 *
 * @param      server               Accessory server.
 */
void HAPAccessoryServerRefreshSetupPayload(HAPAccessoryServer* server);

/**
 * Duration after which NFC pairing mode exits automatically.
 *
 * @see HomeKit Accessory Protocol Specification R17
 *      Section 4.4.2.1 Requirements
 */
#define kHAPAccessoryServer_NFCPairingModeDuration ((HAPTime)(5 * HAPMinute))

/**
 * Enters the pairing mode for programmable NFC tags.
 * NFC pairing mode exits automatically after 5 minutes or when pairing completes.
 *
 * - Note that displays are always updated with a setup payload, even when NFC pairing mode is not entered.
 *   For security reasons, entering NFC pairing mode must require user interaction.
 *
 * @param      server               Accessory server.
 */
void HAPAccessoryServerEnterNFCPairingMode(HAPAccessoryServer* server);

/**
 * Exits the pairing mode for programmable NFC tags.
 *
 * @param      server               Accessory server.
 */
void HAPAccessoryServerExitNFCPairingMode(HAPAccessoryServer* server);

/**
 * Enters the WAC mode.
 * In Wi-Fi Accessory Configuration (WAC) mode, the accessory accepts the configuration of Wi-Fi credentials. If the
 * accessory is already configured when entering WAC mode, the existing Wi-Fi configuration will be deleted first.
 * During WAC mode, the normal HAP accessory server functionality is not available. The accessory server will exit WAC
 * mode as soon as a valid Wi-Fi configuration transaction has been completed. The accessory server will then enter
 * normal HAP mode. The accessory server will also exit WAC mode in response to calling the function
 * HAPAccessoryServerExitWACMode or if no valid configuration requests have been received for more than 15 minutes. In
 * this case, the accessory server keeps running non configured and it is possible to enter WAC mode again with the
 * function HAPAccessoryServerEnterWACMode in response to a direct user action.
 *
 * If a Wi-Fi configuration has been applied through an out-of-band mechanism (not via WAC), it is necessary
 * to call HAPAccessoryServerExitWACMode. The accessory server will then enter normal HAP mode.
 *
 * Remark: The accessory server will enter WAC mode automatically when it is started in a non-configured state.
 *
 * This function only has an effect if:
 * - WAC is supported by the platform.
 * - The accessory server is not already in WAC mode, see HAPAccessoryServerIsInWACMode.
 *
 * @param      server               Accessory server.
 */
void HAPAccessoryServerEnterWACMode(HAPAccessoryServer* server);

/**
 * Exits the WAC mode.
 * If a Wi-Fi configuration has been applied, the accessory server will enter normal HAP mode.
 * Otherwise the accessory server keeps running non configured and it is possible to enter WAC mode again
 * with the function HAPAccessoryServerEnterWACMode in response to a direct user action.
 *
 * This function only has an effect if:
 * - WAC is supported by the platform.
 * - The accessory server is in WAC mode, see HAPAccessoryServerIsInWACMode.
 *
 * @param      server               Accessory server.
 */
void HAPAccessoryServerExitWACMode(HAPAccessoryServer* server);

/**
 * Raises an event notification for a given characteristic in a given service provided by a given accessory object.
 *
 * @param      server               Accessory server.
 * @param      characteristic       The characteristic whose value has changed.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 */
void HAPAccessoryServerRaiseEvent(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory);

/**
 * Sets whether Thread network was reachable.
 *
 * Network being found means either reachability test passed or Thread joiner succeeded.
 *
 * @param server        accessory server
 * @param reachable     true if the network was reachable. false, otherwise.
 * @param securityError true if a security error occurred during the test. false, otherwise.
 */
void HAPAccessoryServerSetThreadNetworkReachable(HAPAccessoryServer* server, bool reachable, bool securityError);

/**
 * Raises an event notification for a given characteristic in a given service provided by a given accessory object
 * on a given session.
 *
 * @param      server               Accessory server.
 * @param      characteristic       The characteristic whose value has changed.
 * @param      service              The service that contains the characteristic.
 * @param      accessory            The accessory that provides the service.
 * @param      session              The session on which to raise the event.
 */
void HAPAccessoryServerRaiseEventOnSession(
        HAPAccessoryServer* server,
        const HAPCharacteristic* characteristic,
        const HAPService* service,
        const HAPAccessory* accessory,
        HAPSession* session);

/**
 * Callback function type to be called when heart beat value changes.
 *
 * @param data    user data setup in HAPAccessoryServerStartHeartBeat().
 */
typedef void (*HAPAccessoryServerHeartBeatCallback)(void* _Nullable data);

/**
 * Start or restart heart beat characteristic processing and register callback to handle the characteristic value
 * change.
 * This function must be called every time, accessory power cycles or factory resets.
 *
 * @param      server               Accessory server
 * @param      callback             callback function
 * @param      data                 user data to pass to the callback function
 *
 * @return kHAPError_None if successful. Other error code if the function failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPAccessoryServerStartHeartBeat(
        HAPAccessoryServer* server,
        HAPAccessoryServerHeartBeatCallback callback,
        void* _Nullable data);

/**
 * Indicates whether the given key-value store is compatible with this version of the HAP library.
 *
 * - If the key-value store is not compatible, the accessory server will not start.
 *   HAPKeystoreRestoreFactorySettings may be used to restore compatibility.
 *
 * @param      keyValueStore        Key-value store.
 *
 * @return true                     If the key-value store is compatible with this version of the HAP library.
 * @return false                    Otherwise. May only occur when the accessory firmware has been downgraded.
 */
HAP_RESULT_USE_CHECK
bool HAPIsKeyValueStoreCompatible(HAPPlatformKeyValueStoreRef keyValueStore);
/**
 * Restores the given key-value store to factory settings.
 *
 * - The accessory server must be stopped completely before restoring factory settings!
 *
 * - Only HomeKit related data is reset. Application specific data must be restored manually.
 *
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPKeystoreRestoreFactorySettings(HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Purges all HomeKit pairings from the given key-value store.
 *
 * - The accessory server must be stopped completely before issuing a pairing reset!
 *
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPKeystoreClearPairings(HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Purges Device ID and Ed25519 long-term secret key from a given key-value store.
 * This function is intended to purge secret data from a key-value store.
 *
 * - The accessory server must be stopped completely before resetting a pairing!
 * - If the identity is reset, the pairings must be reset as well.
 *
 * @param      keyValueStore        Key-value store.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If persistent store access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPKeystoreResetIdentity(HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Device ID of an accessory server.
 */
typedef struct {
    /**
     * Device ID.
     */
    uint8_t bytes[6];
} HAPAccessoryServerDeviceID;
HAP_STATIC_ASSERT(sizeof(HAPAccessoryServerDeviceID) == 6, HAPAccessoryServerDeviceID);

/**
 * Imports a Device ID into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      deviceID             Device ID of the accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportDeviceID(HAPPlatformKeyValueStoreRef keyValueStore, const HAPAccessoryServerDeviceID* deviceID);

#if (HAP_FEATURE_KEY_EXPORT == 1)
/**
 * Exports the Device ID from a key-value store.
 *
 * - If an accessory server is running, this function must be called from the same run loop.
 *   This function may also be called without creating an accessory server.
 *
 * @param      keyValueStore        Key-value store.
 * @param[out] valid                Whether or not a Device ID is available.
 * @param[out] deviceID             Device ID of the accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError
        HAPExportDeviceID(HAPPlatformKeyValueStoreRef keyValueStore, bool* valid, HAPAccessoryServerDeviceID* deviceID);
#endif

/**
 * Imports a configuration number into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      configurationNumber  Configuration number.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportConfigurationNumber(HAPPlatformKeyValueStoreRef keyValueStore, uint32_t configurationNumber);

/**
 * Ed25519 long-term secret key of an accessory server.
 */
typedef struct {
    /**
     * Ed25519 long-term secret key.
     */
    uint8_t bytes[32];
} HAPAccessoryServerLongTermSecretKey;
HAP_STATIC_ASSERT(sizeof(HAPAccessoryServerLongTermSecretKey) == 32, HAPAccessoryServerLongTermSecretKey);

/**
 * Imports a Ed25519 long-term secret key into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      longTermSecretKey    Long-term secret key of the accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportLongTermSecretKey(
        HAPPlatformKeyValueStoreRef keyValueStore,
        const HAPAccessoryServerLongTermSecretKey* longTermSecretKey);

#if (HAP_FEATURE_KEY_EXPORT == 1)
/**
 * Exports the Ed25519 long-term secret key from a key-value store.
 *
 * - If an accessory server is running, this function must be called from the same run loop.
 *   This function may also be called without creating an accessory server.
 *
 * @param      keyValueStore        Key-value store.
 * @param[out] valid                Whether or not a long-term secret key is available.
 * @param[out] longTermSecretKey    Long-term secret key of the accessory server.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError HAPExportLongTermSecretKey(
        HAPPlatformKeyValueStoreRef keyValueStore,
        bool* valid,
        HAPAccessoryServerLongTermSecretKey* longTermSecretKey);
#endif

/**
 * Imports an unsuccessful authentication attempts counter into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      numAuthAttempts      Unsuccessful authentication attempts counter.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportUnsuccessfulAuthenticationAttemptsCounter(
        HAPPlatformKeyValueStoreRef keyValueStore,
        uint8_t numAuthAttempts);

/**
 * Imports a controller pairing into an un-provisioned key-value store.
 * This is useful to import legacy settings from a different HomeKit SDK.
 *
 * - This function must no longer be called after the initial HAPAccessoryServerCreate call.
 *
 * @param      keyValueStore        Un-provisioned key-value store.
 * @param      pairingIndex         Key-value store pairing index. 0 ..< Max number of pairings that will be supported.
 * @param      pairingIdentifier    HomeKit pairing identifier.
 * @param      publicKey            Ed25519 long-term public key of the paired controller.
 * @param      isAdmin              Whether or not the added controller has admin permissions.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError HAPLegacyImportControllerPairing(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreKey pairingIndex,
        const HAPControllerPairingIdentifier* pairingIdentifier,
        const HAPControllerPublicKey* publicKey,
        bool isAdmin);

#if (HAP_FEATURE_KEY_EXPORT == 1)
/**
 * Callback that should be invoked for each paired controller.
 *
 * @param      context              Context.
 * @param      keyValueStore        Key-value store.
 * @param      pairingIdentifier    Pairing identifier.
 * @param      publicKey            Controller public key.
 * @param      isAdmin              Whether or not the added controller has admin permissions.
 * @param[in,out] shouldContinue    True if enumeration shall continue, False otherwise. Is set to true on input.
 */
typedef void (*HAPExportControllerPairingsCallback)(
        void* _Nullable context,
        HAPPlatformKeyValueStoreRef keyValueStore,
        const HAPControllerPairingIdentifier* pairingIdentifier,
        const HAPControllerPublicKey* publicKey,
        bool isAdmin,
        bool* shouldContinue);

/**
 * Exports the controller pairings from a key-value store.
 *
 * - If an accessory server is running, this function must be called from the same run loop.
 *   This function may also be called without creating an accessory server.
 *
 * @param      keyValueStore        Key-value store.
 * @param      callback             Function to call on each paired controller.
 * @param      context              Context that is passed to the callback.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If an error occurred while accessing the key-value store.
 */
HAP_RESULT_USE_CHECK
HAPError HAPExportControllerPairings(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPExportControllerPairingsCallback callback,
        void* _Nullable context);
#endif

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
