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

#ifndef HAP_PLATFORM_BLE_PERIPHERAL_MANAGER_H
#define HAP_PLATFORM_BLE_PERIPHERAL_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPBase.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * @file
 *
 * A BLE accessory comes with a BLE stack, which is interfaced through this PAL module. The PAL implementation must wrap
 * the BLE stack functionality for advertising, services, characteristics, and read/write events.
 *
 * **Expected behavior:**
 *
 * The requirements for a BLE stack are as follows:
 * - The BLE stack must allow the HAP Library to handle read and write requests directly. This is required for HomeKit
 * session security. For read requests, the response value and length are dynamically computed on a per-request basis.
 * For write requests, the application must be able to decide whether an error response or a write response is
 * return­ed. Therefore, the stack must not automatically respond with cached values.
 * - The BLE stack must allow delaying the response to a read or write request by several seconds.
 * - It must be possible to disconnect from the central without responding to a read or write request. It must be
 * possible to immediately disconnect after a read response has been transmitted.
 * - It is recommended that the GATT database size be configurable by the application and that characteristic values be
 * stored in application memory. It is also recommended to store 128-bit UUIDs in a compact format. The same descriptor
 * UUIDs are used for each characteristic and Apple-defined characteristics use 128-bit UUIDs based on a common base
 * UUID (similar to the way 16-bit UUIDs assigned by the Bluetooth SIG operate).
 *
 * To interface with the BLE stack, about a dozen functions must be implemented, for managing device addresses and
 * names, advertisements and connections, and for exposing services, characteristics and descriptors.
 */

/**
 * BLE peripheral manager.
 */
typedef struct HAPPlatformBLEPeripheralManager HAPPlatformBLEPeripheralManager;
typedef struct HAPPlatformBLEPeripheralManager* HAPPlatformBLEPeripheralManagerRef;
HAP_NONNULL_SUPPORT(HAPPlatformBLEPeripheralManager)

/**
 * Bluetooth Connection Handle.
 *
 * - Range: 0x0000-0x0EFF.
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 2 Part E Section 5.3.1 Primary Controller Handles
 */
typedef uint16_t HAPPlatformBLEPeripheralManagerConnectionHandle;

/**
 * Bluetooth Attribute Handle.
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 3 Part F Section 3.2.2 Attribute Handle
 */
typedef uint16_t HAPPlatformBLEPeripheralManagerAttributeHandle;

/**
 * Maximum length of an attribute value.
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 3 Part F Section 3.2.9 Long Attribute Values
 */
#define kHAPPlatformBLEPeripheralManager_MaxAttributeBytes ((size_t) 512)

/**
 * Delegate that is used to monitor read, write, and subscription requests from remote central devices.
 */
typedef struct {
    /**
     * Client context pointer. Will be passed to callbacks.
     */
    void* _Nullable context;

    /**
     * Invoked when a connection has been established in response to the advertising data that has been set
     * through HAPPlatformBLEPeripheralManagerStartAdvertising.
     *
     * - If a connection is established through other means, it is not considered a HomeKit connection
     *   and must not lead to the invocation of this callback.
     *
     * @param      blePeripheralManager BLE peripheral manager.
     * @param      connectionHandle     Connection handle of the connected central.
     * @param      context              The context pointer of the BLE peripheral manager delegate structure.
     */
    void (*_Nullable handleConnectedCentral)(
            HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
            HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
            void* _Nullable context);

    /**
     * Invoked when a connection that was reported to the handleConnectedCentral callback has been terminated.
     *
     * @param      blePeripheralManager BLE peripheral manager.
     * @param      connectionHandle     Connection handle of the disconnected central.
     * @param      context              The context pointer of the BLE peripheral manager delegate structure.
     */
    void (*_Nullable handleDisconnectedCentral)(
            HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
            HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
            void* _Nullable context);

    /**
     * Invoked when a read request is received on an attribute that has been registered through
     * HAPPlatformBLEPeripheralManagerAddCharacteristic or HAPPlatformBLEPeripheralManagerAddDescriptor.
     *
     * - The supplied buffer should have space for kHAPPlatformBLEPeripheralManager_MaxAttributeBytes bytes.
     *   It is left to the BLE peripheral manager implementation to then transfer the full buffer over a sequence of
     *   central-initiated "Read Request" and "Read Blob Request" operations to the central.
     *   This callback should only be invoked again once the full data has been transmitted.
     *
     * @param      blePeripheralManager BLE peripheral manager.
     * @param      connectionHandle     Connection handle of the central that sent the request.
     * @param      attributeHandle      Attribute handle that is being read.
     * @param[out] bytes                Buffer to fill read response into.
     * @param      maxBytes             Capacity of buffer.
     * @param[out] numBytes             Length of data that was filled into buffer.
     * @param      context              The context pointer of the BLE peripheral manager delegate structure.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_InvalidState   If a read on that characteristic is not allowed in the current state.
     * @return kHAPError_OutOfResources If the supplied buffer is not large enough.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*_Nullable handleReadRequest)(
            HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
            HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
            HAPPlatformBLEPeripheralManagerAttributeHandle attributeHandle,
            void* bytes,
            size_t maxBytes,
            size_t* numBytes,
            void* _Nullable context);

    /**
     * Invoked when a write request is received on an attribute that has been registered through
     * HAPPlatformBLEPeripheralManagerAddCharacteristic or HAPPlatformBLEPeripheralManagerAddDescriptor.
     *
     * - The supplied buffer must support writes up to kHAPPlatformBLEPeripheralManager_MaxAttributeBytes bytes.
     *   It is left to the BLE peripheral manager implementation to assemble fragments of potential
     *   "Prepare Write Request" and "Execute Write Request" operations.
     *   This callback should only be invoked once the full data has been received.
     *
     * @param      blePeripheralManager BLE peripheral manager.
     * @param      connectionHandle     Connection handle of the central that sent the request.
     * @param      attributeHandle      Attribute handle that is being read.
     * @param      bytes                Buffer that contains the request data.
     * @param      numBytes             Length of data in buffer.
     * @param      context              The context pointer of the BLE peripheral manager delegate structure.
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_InvalidState   If a write on that characteristic is not allowed in the current state.
     * @return kHAPError_InvalidData    If the request data has an invalid format.
     * @return kHAPError_OutOfResources If there are not enough resources to handle the request.
     */
    HAP_RESULT_USE_CHECK
    HAPError (*_Nullable handleWriteRequest)(
            HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
            HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
            HAPPlatformBLEPeripheralManagerAttributeHandle attributeHandle,
            void* bytes,
            size_t numBytes,
            void* _Nullable context);

    /**
     * Invoked when the BLE peripheral manager is again ready to send characteristic value updates through
     * HAPPlatformBLEPeripheralManagerSendHandleValueIndication.
     *
     * @param      blePeripheralManager BLE peripheral manager.
     * @param      connectionHandle     Connection handle of the central that is ready to receive events.
     * @param      context              The context pointer of the BLE peripheral manager delegate structure.
     */
    void (*_Nullable handleReadyToUpdateSubscribers)(
            HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
            HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
            void* _Nullable context);
} HAPPlatformBLEPeripheralManagerDelegate;

/**
 * Specifies or clears the delegate for receiving peripheral events.
 *
 * - The delegate structure is copied and does not need to remain valid.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      delegate             The delegate to receive the peripheral role events. NULL to clear.
 */
void HAPPlatformBLEPeripheralManagerSetDelegate(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerDelegate* _Nullable delegate);

/**
 * Bluetooth device address (BD_ADDR).
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 2 Part B Section 1.2 Bluetooth Device Addressing
 */
typedef struct {
    uint8_t bytes[6]; /**< Little-endian. */
} HAPPlatformBLEPeripheralManagerDeviceAddress;
HAP_STATIC_ASSERT(
        sizeof(HAPPlatformBLEPeripheralManagerDeviceAddress) == 6,
        HAPPlatformBLEPeripheralManagerDeviceAddress);

/**
 * Sets the Bluetooth device address (BD_ADDR).
 *
 * - The address is a random (static) MAC address.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      deviceAddress        Bluetooth device address.
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 6 Part B Section 1.3.2.1 Static Device Address
 */
void HAPPlatformBLEPeripheralManagerSetDeviceAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress);

/**
 * Retrieve the device address used in the advertisements throughout the current power cycle.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param[out] deviceAddress        Bluetooth device address.
 *
 * @return @ref kHAPError_None if the BD_ADDR is available to read back in the platform
 *         and the address is used in advertisements throughout the current power cycle.<br>
 *         @ref kHAPError_Unknown, otherwise.
 *         Note that if the platform generates and uses a random address
 *         per advertisement instance, this function must return @ref kHAPError_Unknown.
 */
HAPError HAPPlatformBLEPeripheralManagerGetAdvertisementAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress);

/**
 * Sets the Bluetooth GAP Device Name.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      deviceName           Bluetooth device name.
 */
void HAPPlatformBLEPeripheralManagerSetDeviceName(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const char* deviceName);

/**
 * 128-bit Bluetooth UUID.
 */
typedef struct {
    uint8_t bytes[16]; /**< Little-endian. */
} HAPPlatformBLEPeripheralManagerUUID;
HAP_STATIC_ASSERT(sizeof(HAPPlatformBLEPeripheralManagerUUID) == 16, HAPPlatformBLEPeripheralManagerUUID);

/**
 * Queries whether services can be refreshed in the current state.
 *
 * Note that certain platform does not allow refreshing set of services.
 * In such platforms, prior to setting the first set of services, this function will return true.
 * But after the services were done with, that is after call to HAPPlatformBLEPeripheralManagerRemoveAllServices(),
 * this function must return false.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 *
 * @return true when service refreshing is allowed. false, otherwise.
 */
bool HAPPlatformBLEPeripheralManagerAllowsServiceRefresh(HAPPlatformBLEPeripheralManagerRef blePeripheralManager);

/**
 * Removes all published services from the local GATT database.
 *
 * - Only services that were added through HAPPlatformBLEPeripheralManager methods are affected.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 */
void HAPPlatformBLEPeripheralManagerRemoveAllServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager);

/**
 * Publishes a service to the local GATT database.
 *
 * - Separate AddCharacteristic calls are used to publish the associated characteristics.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      type                 The Bluetooth-specific 128-bit UUID that identifies the service.
 * @param      isPrimary            Whether the type of service is primary or secondary.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If there are not enough resources to publish the service.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddService(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        bool isPrimary);

/**
 * Possible properties of a characteristic.
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 3 Part G Section 3.3.1.1 Characteristic Properties
 */
typedef struct {
    /** If set, permits reads of the Characteristic Value. */
    bool read : 1;

    /** If set, permit writes of the Characteristic Value without response. */
    bool writeWithoutResponse : 1;

    /** If set, permits writes of the Characteristic Value with response. */
    bool write : 1;

    /**
     * If set, permits notifications of a Characteristic Value without acknowledgment.
     * If set, the Client Characteristic Configuration Descriptor must be published as well.
     */
    bool notify : 1;

    /**
     * If set, permits indications of a Characteristic Value with acknowledgment.
     * If set, the Client Characteristic Configuration Descriptor must be published as well.
     */
    bool indicate : 1;
} HAPPlatformBLEPeripheralManagerCharacteristicProperties;

/**
 * Publishes a characteristic to the local GATT database. It is associated with the most recently added service.
 *
 * - Separate AddDescriptor calls are used to publish the associated descriptors.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      type                 The Bluetooth-specific 128-bit UUID that identifies the characteristic.
 * @param      properties           The properties of the characteristic.
 * @param      constBytes           Value if constant, otherwise NULL
 * @param      constNumBytes        Size if constant, otherwise 0
 * @param[out] valueHandle          Attribute handle of the added Characteristic Value declaration.
 * @param[out] cccDescriptorHandle  Attribute handle of the added Client Characteristic Configuration descriptor.
 *                                  This parameter must only be filled if notify or indicate properties are set.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If there are not enough resources to publish the characteristic.
 *
 * @see Bluetooth Core Specification Version 5
 *      Vol 3 Part G Section 3.3.3.3 Client Characteristic Configuration
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddCharacteristic(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        HAPPlatformBLEPeripheralManagerCharacteristicProperties properties,
        const void* _Nullable constBytes,
        size_t constNumBytes,
        HAPPlatformBLEPeripheralManagerAttributeHandle* valueHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle* _Nullable cccDescriptorHandle);

/**
 * Possible properties of a descriptor.
 */
typedef struct {
    /** If set, permits reads of the descriptor. */
    bool read : 1;

    /** If set, permits writes of the descriptor. */
    bool write : 1;
} HAPPlatformBLEPeripheralManagerDescriptorProperties;

/**
 * Publishes a descriptor to the local GATT database. It is associated with the most recently added characteristic.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      type                 The Bluetooth-specific 128-bit UUID that identifies the descriptor.
 * @param      properties           The properties of the descriptor.
 * @param      constBytes           Value if constant, otherwise NULL
 * @param      constNumBytes        Size if constant, otherwise 0*
 * @param[out] descriptorHandle     Attribute handle of the added descriptor.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If there are not enough resources to publish the descriptor.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddDescriptor(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        HAPPlatformBLEPeripheralManagerDescriptorProperties properties,
        const void* _Nullable constBytes,
        size_t constNumBytes,
        HAPPlatformBLEPeripheralManagerAttributeHandle* descriptorHandle);

/**
 * This function is called after all services have been added.
 *
 * - Before new services are added again, HAPPlatformBLEPeripheralManagerRemoveAllServices is called.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 */
void HAPPlatformBLEPeripheralManagerPublishServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager);

/**
 * Advertises BLE peripheral manager data or updates advertised data.
 *
 * - Advertisements must be undirected and connectable (ADV_IND).
 *
 * - When a central connects in response to the advertisements,
 *   the delegate's handleConnectedCentral method shall be called.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      advertisingInterval  Advertisement interval.
 * @param      advertisingBytes     Buffer containing advertising data.
 * @param      numAdvertisingBytes  Length of advertising data buffer.
 * @param      scanResponseBytes    Buffer containing scan response data, if applicable.
 * @param      numScanResponseBytes Length of scan response data buffer.
 */
void HAPPlatformBLEPeripheralManagerStartAdvertising(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPBLEAdvertisingInterval advertisingInterval,
        const void* advertisingBytes,
        size_t numAdvertisingBytes,
        const void* _Nullable scanResponseBytes,
        size_t numScanResponseBytes);

/**
 * Stops advertising BLE peripheral manager data.
 *
 * - Once this function returns, the delegate's handleConnectedCentral method must not be called anymore
 *   unless advertisements are started again.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 */
void HAPPlatformBLEPeripheralManagerStopAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager);

/**
 * Cancels an active connection to a central.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      connectionHandle     Connection handle of the central to disconnect.
 */
void HAPPlatformBLEPeripheralManagerCancelCentralConnection(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle);

/**
 * Sends an indication to a subscribed central to update a characteristic value.
 *
 * @param      blePeripheralManager BLE peripheral manager.
 * @param      connectionHandle     Connection handle of the central to update.
 * @param      valueHandle          Handle of the Characteristic Value declaration whose value changed.
 * @param      bytes                Buffer containing the value that is sent to the central.
 * @param      numBytes             Length of buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_InvalidState   If the event could not be sent at this time, or the central is not subscribed.
 * @return kHAPError_OutOfResources If the buffer is too large to send.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerSendHandleValueIndication(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
        const void* _Nullable bytes,
        size_t numBytes) HAP_DIAGNOSE_ERROR(!bytes && numBytes, "empty buffer cannot have a length");

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
