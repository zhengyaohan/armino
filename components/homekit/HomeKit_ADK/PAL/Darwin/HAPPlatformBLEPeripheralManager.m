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
#import <CoreBluetooth/CoreBluetooth.h>

#include "HAPPlatformBLEPeripheralManager+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "BLEPeripheralManager" };

extern NSString* const CBAdvertisementDataAppleMfgData;
extern NSString* const CBManagerIsPrivilegedDaemonKey;
extern NSString* const CBCentralManagerScanOptionAllowDuplicatesKey;

// Maximum number of peripheral instances - 5 is arbitrary.
#define MAXIMUM_NUM_PERIPHERALS 5

static id peek(NSMutableArray* array) {
    return array.firstObject;
}

static id pop(NSMutableArray* array) {
    id obj = peek(array);
    if (obj) {
        [array removeObjectAtIndex:0];
    }
    return obj;
}

@interface HAPBLEElementWrapper : NSObject<NSCopying>
@property (readonly, nonatomic, retain) NSObject* element;

- (id)initWithElement:(NSObject*)element;
@end

@implementation HAPBLEElementWrapper
- (instancetype)initWithElement:(CBCharacteristic*)element {
    if (self = [super init]) {
        _element = element;
    }
    return self;
}

- (id)copyWithZone:(NSZone*)zone {
    (void) zone;
    id copy = [[HAPBLEElementWrapper alloc] initWithElement:_element];
    return copy;
}

- (BOOL)isEqual:(id)object {
    HAPBLEElementWrapper* other = (HAPBLEElementWrapper*) object;
    // bool output = [other element] == [self element];
    bool output = [[other element] isEqual:[self element]];
    // bool output = [[other hash] isEqual:[self hash]];
    return (output);
}

- (NSUInteger)hash {
    return [[self element] hash];
}

@end

@interface HAPBLEPeripheralDarwin : NSObject<CBPeripheralManagerDelegate> {
}
@end

@implementation HAPBLEPeripheralDarwin {
    HAPPlatformBLEPeripheralManagerRef _hapBLEPeripheralManager;
    HAPPlatformBLEPeripheralManagerDelegate _hapBLEPeripheralManagerDelegate;
    CBPeripheralManager* _peripheralManager;
    NSMutableArray<id>* _pending;
    __weak CBMutableService* _pendingService;
    __weak CBMutableCharacteristic* _pendingCharacteristic;
    NSMutableDictionary<NSObject*, NSNumber*>* _handles;
    NSMutableDictionary<NSString*, CBMutableCharacteristic*>* _characteristics;
    HAPBLEAdvertisingInterval _advertisingInterval;
    NSData* _advertisingData;
    NSData* _scanResponseData;
    uint16_t _nextHandle;
    CBCentral* _connectedCentral;
    struct {
        uint8_t bytes[kHAPPlatformBLEPeripheralManager_MaxAttributeBytes];
        uint16_t handle;
        size_t numBytes;
    } _gatt;
}

- (instancetype)init {
    if (self = [super init]) {
        _hapBLEPeripheralManager = NULL;
        HAPRawBufferZero(&_hapBLEPeripheralManagerDelegate, sizeof _hapBLEPeripheralManagerDelegate);
        _peripheralManager = nil;
        _advertisingInterval = 0;
        _advertisingData = nil;
        ;
        _scanResponseData = nil;
        _connectedCentral = nil;
        [self removeAllServices];
    }
    return self;
}

- (NSString*)getCCCDescriptorKeyForCharacteristic:(CBCharacteristic*)characteristic {
    NSString* cccDescriptorKey =
            [NSString stringWithFormat:@"%@-cccdescriptor", @([self getHandleForObject:characteristic]).stringValue];
    return cccDescriptorKey;
}
- (void)setHAPContainer:(HAPPlatformBLEPeripheralManagerRef)container {
    _hapBLEPeripheralManager = container;
}

- (void)setHAPDelegate:(const HAPPlatformBLEPeripheralManagerDelegate* _Nullable)delegate {
    if (delegate != NULL) {
        _hapBLEPeripheralManagerDelegate = *delegate;
    } else {
        HAPRawBufferZero(&_hapBLEPeripheralManagerDelegate, sizeof _hapBLEPeripheralManagerDelegate);
    }
}

- (void)addService:(CBMutableService*)service {
    [_pending addObject:(_pendingService = service)];
    _pendingCharacteristic = nil;
}

- (uint16_t)addCharacteristic:(CBMutableCharacteristic*)characteristic {
    HAPAssert(_pendingService);

    [_pending addObject:(_pendingCharacteristic = characteristic)];
    return [self assignValueHandleToCharacteristic:characteristic];
}

- (uint16_t)addCCCDescriptorForCharacteristic:(CBMutableCharacteristic*)characteristic handle:(uint16_t)handle {
    HAPAssert(_pendingService);

    return [self assignCCCDescriptorHandleToCharacteristic:characteristic handle:handle];
}

- (uint16_t)addDescriptor:(CBMutableDescriptor*)descriptor {
    HAPAssert(_pendingService);
    HAPAssert(_pendingCharacteristic);

    [_pending addObject:descriptor];
    return [self assignHandleToDescriptor:descriptor];
}

- (void)publishServices {
    _peripheralManager = [[CBPeripheralManager alloc] initWithDelegate:self
                                                                 queue:nil
                                                               options:@{
                                                                   CBManagerIsPrivilegedDaemonKey: @(YES),
                                                                   CBCentralManagerScanOptionAllowDuplicatesKey: @(YES),
                                                               }];
}

- (void)removeAllServices {
    _pending = [[NSMutableArray alloc] init];
    _pendingService = nil;
    _pendingCharacteristic = nil;
    _nextHandle = 1;
    _handles = [[NSMutableDictionary alloc] init];
    _characteristics = [[NSMutableDictionary alloc] init];

    if (_peripheralManager) {
        [_peripheralManager stopAdvertising];
        [_peripheralManager removeAllServices];
        _peripheralManager = nil;
    }
}

- (void)startAdvertising:(HAPBLEAdvertisingInterval)advertisingInterval
         advertisingData:(NSData*)advertisingData
            scanResponse:(NSData*)scanResponseData {
    _advertisingInterval = advertisingInterval;
    _advertisingData = advertisingData;
    _scanResponseData = scanResponseData;

    [self updateState];
}

- (void)stopAdvertising {
    _advertisingInterval = 0;
    _advertisingData = nil;
    _scanResponseData = nil;

    [self updateState];
}

- (void)updateState {
    if (_peripheralManager.state != CBManagerStatePoweredOn) {
        HAPLog(&logObject, "Not powered on.");
        return;
    }

    while (peek(_pending)) {
        HAPAssert([peek(_pending) isKindOfClass:CBMutableService.class]);
        CBMutableService* service = pop(_pending);
        NSMutableArray* characteristics = [[NSMutableArray alloc] init];
        while ([peek(_pending) isKindOfClass:CBMutableCharacteristic.class]) {
            CBMutableCharacteristic* characteristic = pop(_pending);
            NSMutableArray* descriptors = [[NSMutableArray alloc] init];
            while ([peek(_pending) isKindOfClass:CBMutableDescriptor.class]) {
                [descriptors addObject:pop(_pending)];
            }
            characteristic.descriptors = descriptors.count ? descriptors : nil;
            [characteristics addObject:characteristic];
        }
        service.characteristics = characteristics;
        [_peripheralManager addService:service];
    }
    _pendingService = nil;
    _pendingCharacteristic = nil;

    if (!_advertisingInterval) {
        if (_peripheralManager.isAdvertising) {
            [_peripheralManager stopAdvertising];
        }

        HAPLog(&logObject, "Not advertising.");
        return;
    }

    HAPLog(&logObject, "Starting to advertise.");

    [_peripheralManager startAdvertising:@{
        CBAdvertisementDataAppleMfgData: _advertisingData,
        CBAdvertisementDataLocalNameKey: _scanResponseData,
    }];
}

- (void)peripheralManagerDidUpdateState:(CBPeripheralManager*)peripheral {
    HAPAssert(peripheral == _peripheralManager);

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, "peripheralManagerDidUpdateState");

        [self updateState];
    });
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral willRestoreState:(NSDictionary<NSString*, id>*)dict {
    HAPAssert(peripheral == _peripheralManager);
    (void) dict;

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, "willRestoreState");
    });
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral didAddService:(CBService*)service error:(NSError*)error {
    HAPAssert(peripheral == _peripheralManager);

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, "didAddService");
    });

    (void) service;
    (void) error;
}

- (void)peripheralManagerDidStartAdvertising:(CBPeripheralManager*)peripheral error:(NSError*)error {
    HAPAssert(peripheral == _peripheralManager);
    (void) error;

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, "peripheralManagerDidStartAdvertising");
    });
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral
                             central:(CBCentral*)central
        didSubscribeToCharacteristic:(CBCharacteristic*)characteristic {
    HAPAssert(peripheral == _peripheralManager);

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, "didSubscribeToCharacteristic");

        uint16_t cccDescriptorHandle = [self getCCCDescriptorHandleForCharacteristic:characteristic];

        uint16_t bytes = 0;
        // Data enables events
        HAPWriteLittleUInt16(&bytes, 0x0002U);

        [self updateCentralConnection:central];

        HAPError err = _hapBLEPeripheralManagerDelegate.handleWriteRequest(
                _hapBLEPeripheralManager,
                [self getHandleForCentral:_connectedCentral],
                cccDescriptorHandle,
                (void*) &bytes,
                2,
                _hapBLEPeripheralManagerDelegate.context);

        if (err) {
            HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
            return;
        }
    });
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral
                                 central:(CBCentral*)central
        didUnsubscribeFromCharacteristic:(CBCharacteristic*)characteristic {
    HAPAssert(peripheral == _peripheralManager);
    (void) central;

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, "didUnsubscribeFromCharacteristic");

        // If the central is nil from previous unsubscribe call already, all the subscribed characteristics
        // are already unsubscribed and hence it is not necessary to unsubscribe individual characteristic.
        if (_connectedCentral) {
            uint16_t cccDescriptorHandle = [self getCCCDescriptorHandleForCharacteristic:characteristic];

            uint16_t bytes = 0;
            // Data disables events
            HAPWriteLittleUInt16(&bytes, 0x0000U);

            HAPError err = _hapBLEPeripheralManagerDelegate.handleWriteRequest(
                    _hapBLEPeripheralManager,
                    [self getHandleForCentral:_connectedCentral],
                    cccDescriptorHandle,
                    (void*) &bytes,
                    2,
                    _hapBLEPeripheralManagerDelegate.context);

            if (err) {
                HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
                return;
            }

            [self updateCentralConnection:nil];
        }
    });
}

- (void)peripheralManagerIsReadyToUpdateSubscribers:(CBPeripheralManager*)peripheral {
    HAPAssert(peripheral == _peripheralManager);

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, "peripheralManagerIsReadyToUpdateSubscribers");
    });
}

- (void)updateCentralConnection:(CBCentral*)central {
    if (central != _connectedCentral) {
        if (_connectedCentral) {
            if (_hapBLEPeripheralManagerDelegate.handleDisconnectedCentral) {
                _hapBLEPeripheralManagerDelegate.handleDisconnectedCentral(
                        _hapBLEPeripheralManager,
                        [self getHandleForCentral:_connectedCentral],
                        _hapBLEPeripheralManagerDelegate.context);
            }
            _connectedCentral = nil;
        }
        if (central) {
            uint16_t connectionHandle = [self assignHandleToCentral:(_connectedCentral = central)];
            HAPRawBufferZero(&_gatt, sizeof _gatt);
            if (_hapBLEPeripheralManagerDelegate.handleConnectedCentral) {
                _hapBLEPeripheralManagerDelegate.handleConnectedCentral(
                        _hapBLEPeripheralManager, connectionHandle, _hapBLEPeripheralManagerDelegate.context);
            }
        }
    }
}

- (void)respondToRequest:(CBATTRequest*)request withResult:(CBATTError)error {
    [_peripheralManager.self respondToRequest:request withResult:error];
    if (error) {
        // Abort the connection if there is an error
        [self updateCentralConnection:nil];
    }
}

- (uint16_t)makeHandle {
    return _nextHandle++;
}

- (void)setHandle:(uint16_t)handle forObject:(NSObject*)obj {
    HAPBLEElementWrapper* wrapper = [[HAPBLEElementWrapper alloc] initWithElement:obj];
    [_handles setObject:@(handle) forKey:wrapper];
}

- (uint16_t)assignHandleToObject:(NSObject*)obj {
    uint16_t handle = [self makeHandle];
    [self setHandle:handle forObject:obj];
    return handle;
}

- (uint16_t)assignHandleToCentral:(CBCentral*)central {
    return [self assignHandleToObject:central];
}

- (uint16_t)assignValueHandleToCharacteristic:(CBMutableCharacteristic*)characteristic {
    uint16_t handle = [self assignHandleToObject:characteristic];
    [_characteristics setValue:characteristic forKey:@(handle).stringValue];
    return handle;
}

- (uint16_t)assignCCCDescriptorHandleToCharacteristic:(CBMutableCharacteristic*)characteristic handle:(uint16_t)handle {
    NSString* cccDescriptorKey = [self getCCCDescriptorKeyForCharacteristic:characteristic];
    [self setHandle:handle forObject:cccDescriptorKey];
    return handle;
}

- (uint16_t)assignHandleToDescriptor:(CBMutableDescriptor*)descriptor {
    return [self assignHandleToObject:descriptor];
}

- (uint16_t)getHandleForObject:(NSObject*)obj {
    HAPBLEElementWrapper* wrapper = [[HAPBLEElementWrapper alloc] initWithElement:obj];
    uint16_t handle = [[_handles objectForKey:wrapper] unsignedShortValue];
    HAPAssert(handle);
    return handle;
}

- (uint16_t)getHandleForCentral:(CBCentral*)central {
    return [self getHandleForObject:central];
}

- (uint16_t)getValueHandleForCharacteristic:(CBCharacteristic*)characteristic {
    return [self getHandleForObject:characteristic];
}

- (uint16_t)getCCCDescriptorHandleForCharacteristic:(CBCharacteristic*)characteristic {
    return [self getHandleForObject:[self getCCCDescriptorKeyForCharacteristic:characteristic]];
}

- (CBMutableCharacteristic*)getCharacteristicForHandle:(uint16_t)handle {
    return [_characteristics valueForKey:@(handle).stringValue];
}

- (bool)updateCharacteristic:(uint16_t)valueHandle withValue:(NSData*)value forConnection:(uint16_t)connectionHandle {
    HAPPrecondition(_connectedCentral);
    HAPPrecondition([self getHandleForCentral:_connectedCentral] == connectionHandle);

    CBMutableCharacteristic* characteristic = [self getCharacteristicForHandle:valueHandle];
    HAPAssert(characteristic);

    return [_peripheralManager updateValue:value
                         forCharacteristic:characteristic
                      onSubscribedCentrals:@[_connectedCentral]];
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral didReceiveReadRequest:(CBATTRequest*)request {
    HAPAssert(peripheral == _peripheralManager);

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, __func__);

        [self updateCentralConnection:request.central];

        uint16_t valueHandle = [self getValueHandleForCharacteristic:request.characteristic];

        if (!request.offset) {
            // Start new read transaction.
            if (!_hapBLEPeripheralManagerDelegate.handleReadRequest) {
                HAPLog(&logObject, "No read request handler plugged in. Sending error response.");
                [self respondToRequest:request withResult:CBATTErrorReadNotPermitted];
                return;
            }

            HAPLogDebug(&logObject, "(0x%04x) ATT Read Request.", valueHandle);

            size_t len;
            HAPError err;
            err = _hapBLEPeripheralManagerDelegate.handleReadRequest(
                    _hapBLEPeripheralManager,
                    [self getHandleForCentral:_connectedCentral],
                    valueHandle,
                    _gatt.bytes,
                    sizeof _gatt.bytes,
                    &len,
                    _hapBLEPeripheralManagerDelegate.context);
            if (err) {
                HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
                [self respondToRequest:request withResult:CBATTErrorInsufficientResources];
                return;
            }

            _gatt.handle = valueHandle;
            HAPAssert(len <= sizeof _gatt.bytes);
            _gatt.numBytes = len;
        } else {
            // Read Blob Request.
            // See Bluetooth Core Specification Version 5
            // Vol 3 Part F Section 3.4.4.5 Read Blob Request
            HAPLogDebug(&logObject, "(0x%04x) ATT Read Blob Request.", valueHandle);

            if (_gatt.handle != valueHandle) {
                HAPLog(&logObject,
                       "Received Read Blob Request for a different characteristic than prior Read Request.");
                [self respondToRequest:request withResult:CBATTErrorRequestNotSupported];
                return;
            }
        }

        HAPAssert(request.offset <= _gatt.numBytes);
        HAPAssert(request.offset >= 0);

        // Send response.
        size_t len = _gatt.numBytes - request.offset;

        // Note that non-zero request.offset is not necessarily where the next data starts over the air,
        // that is, MTU plus the last request.offset. It seems that CoreBluetooth buffers a few more bytes than MTU
        // and dispatches didReceiveReadRequest: again with the next offset to queue.
        // Also note that when calling respondToRequest:withResult:, data should be the full length remaining data
        // and must not be cut off by MTU length.
        HAPLogDebug(&logObject, "offset=%zu, len=%zu", request.offset, len);

        request.value = [NSData dataWithBytes:(_gatt.bytes + request.offset) length:len];
        [self respondToRequest:request withResult:CBATTErrorSuccess];
    });
}

- (void)peripheralManager:(CBPeripheralManager*)peripheral didReceiveWriteRequests:(NSArray<CBATTRequest*>*)requests {
    HAPAssert(peripheral == _peripheralManager);

    dispatch_async(dispatch_get_main_queue(), ^{
        HAPLog(&logObject, __func__);

        for (CBATTRequest* request in requests) {
            [self updateCentralConnection:request.central];

            uint16_t valueHandle = [self getValueHandleForCharacteristic:request.characteristic];

            HAPError err = _hapBLEPeripheralManagerDelegate.handleWriteRequest(
                    _hapBLEPeripheralManager,
                    [self getHandleForCentral:_connectedCentral],
                    valueHandle,
                    (void*) request.value.bytes,
                    request.value.length,
                    _hapBLEPeripheralManagerDelegate.context);
            if (err) {
                HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
                [self respondToRequest:request withResult:CBATTErrorInvalidPdu];
                return;
            }
        }

        [self respondToRequest:requests[0] withResult:CBATTErrorSuccess];
    });
}

@end

/** Internal representation of the HAPlatformBLEPeripheralManager */
typedef struct {
    CFTypeRef obj; /**< owner pointer to HAPBLEPeripheralDarwin */
} HAPPlatformBLEPeripheralManagerInternal;
static_assert(
        sizeof(HAPPlatformBLEPeripheralManagerInternal) <= sizeof(HAPPlatformBLEPeripheralManager),
        "HAPPlatformBLEPeripheralManager should be large enough to contain implementation");

/**
 * List of all peripheral instances, in order to destroy all peripherals when application aborts.
 *
 * This is necessary because if peripheral instance were owned by C application (ownership transferred),
 * upon aborting the application, the peripheral objects would not be destroyed leaving the BLE advertisement alive, for
 * instance.
 */
static HAPBLEPeripheralDarwin* peripherals[MAXIMUM_NUM_PERIPHERALS];

void HAPPlatformBLEPeripheralManagerCreate(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerOptions* options) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPPrecondition(options);
    HAPPrecondition(options->keyValueStore);

    size_t i;
    for (i = 0; i < HAPArrayCount(peripherals); i++) {
        if (peripherals[i] == nil) {
            break;
        }
    }
    if (i == HAPArrayCount(peripherals)) {
        HAPLogError(&logObject, "Out of resources for HAPPlatformBLEPeripheralManager instances");
        HAPFatalError();
    }
    peripherals[i] = [[HAPBLEPeripheralDarwin alloc] init];
    [peripherals[i] setHAPContainer:blePeripheralManager_];
    blePeripheralManager->obj = (__bridge CFTypeRef) peripherals[i];
}

void HAPPlatformBLEPeripheralManagerSetDelegate(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerDelegate* _Nullable delegate) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;

    [peripheral setHAPDelegate:delegate];
}

void HAPPlatformBLEPeripheralManagerSetDeviceAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(deviceAddress);

    HAPLog(&logObject, __func__);
}

HAPError HAPPlatformBLEPeripheralManagerGetAdvertisementAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager HAP_UNUSED,
        HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress HAP_UNUSED) {
    // Darwin doesn't use a permanent address in advertisement.
    return kHAPError_Unknown;
}

void HAPPlatformBLEPeripheralManagerSetDeviceName(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const char* deviceName) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(deviceName);

    HAPLog(&logObject, __func__);
}

static void memrev(uint8_t* dst, const uint8_t* src, size_t n) {
    src += n;
    while (n--) {
        *dst++ = *--src;
    }
}

bool HAPPlatformBLEPeripheralManagerAllowsServiceRefresh(HAPPlatformBLEPeripheralManagerRef blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);
    return true;
}

static CBUUID* uuid(const HAPPlatformBLEPeripheralManagerUUID* uuid) {
    NSMutableData* data = [NSMutableData dataWithLength:sizeof(*uuid)];
    memrev((uint8_t*) data.bytes, (const uint8_t*) uuid, sizeof(*uuid));
    return [CBUUID UUIDWithData:data];
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddService(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        bool isPrimary) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;
    HAPPrecondition(type);

    HAPLog(&logObject, __func__);

    [peripheral addService:[[CBMutableService alloc] initWithType:uuid(type) primary:isPrimary]];

    return kHAPError_None;
}

void HAPPlatformBLEPeripheralManagerRemoveAllServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager_) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;

    HAPLog(&logObject, __func__);

    [peripheral removeAllServices];
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddCharacteristic(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        HAPPlatformBLEPeripheralManagerCharacteristicProperties properties,
        const void* _Nullable constBytes,
        size_t constNumBytes,
        HAPPlatformBLEPeripheralManagerAttributeHandle* valueHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle* _Nullable cccDescriptorHandle) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;
    HAPPrecondition(type);
    HAPPrecondition((!constBytes && !constNumBytes) || (constBytes && constNumBytes));
    HAPPrecondition(valueHandle);
    if (properties.notify || properties.indicate) {
        HAPPrecondition(cccDescriptorHandle);
    } else {
        HAPPrecondition(!cccDescriptorHandle);
    }

    HAPLog(&logObject, __func__);

    CBCharacteristicProperties prop = 0;
    if (properties.read)
        prop |= CBCharacteristicPropertyRead;
    if (properties.write)
        prop |= CBCharacteristicPropertyWrite;
    if (properties.writeWithoutResponse)
        prop |= CBCharacteristicPropertyWriteWithoutResponse;
    if (properties.notify)
        prop |= CBCharacteristicPropertyNotify;
    if (properties.indicate)
        prop |= CBCharacteristicPropertyIndicate;
    CBAttributePermissions perm = 0;
    if (properties.read || properties.notify || properties.indicate)
        perm |= CBAttributePermissionsReadable;
    if (properties.write || properties.writeWithoutResponse)
        perm |= CBAttributePermissionsWriteable;

    NSData* value = constBytes ? [NSData dataWithBytes:constBytes length:constNumBytes] : nil;
    CBMutableCharacteristic* characteristic = [[CBMutableCharacteristic alloc] initWithType:uuid(type)
                                                                                 properties:prop
                                                                                      value:value
                                                                                permissions:perm];
    *valueHandle = [peripheral addCharacteristic:characteristic];

    if (properties.notify || properties.indicate) {
        *cccDescriptorHandle = [peripheral makeHandle];
        [peripheral addCCCDescriptorForCharacteristic:characteristic handle:*cccDescriptorHandle];
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddDescriptor(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        HAPPlatformBLEPeripheralManagerDescriptorProperties properties HAP_UNUSED,
        const void* _Nullable constBytes,
        size_t constNumBytes,
        HAPPlatformBLEPeripheralManagerAttributeHandle* descriptorHandle) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;
    HAPPrecondition(type);
    HAPPrecondition(constBytes && constNumBytes);
    HAPPrecondition(descriptorHandle);

    HAPLog(&logObject, __func__);

    NSData* value = constBytes ? [NSData dataWithBytes:constBytes length:constNumBytes] : nil;
    CBMutableDescriptor* descriptor = [[CBMutableDescriptor alloc] initWithType:uuid(type) value:value];
    *descriptorHandle = [peripheral addDescriptor:descriptor];

    return kHAPError_None;
}

void HAPPlatformBLEPeripheralManagerPublishServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager_) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;

    HAPLog(&logObject, __func__);

    if (!peripheral) {
        return;
    }

    [peripheral publishServices];
}

void HAPPlatformBLEPeripheralManagerStartAdvertising(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        HAPBLEAdvertisingInterval advertisingInterval,
        const void* advertisingBytes,
        size_t numAdvertisingBytes,
        const void* _Nullable scanResponseBytes,
        size_t numScanResponseBytes) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;
    HAPPrecondition(advertisingInterval);
    HAPPrecondition(advertisingBytes);
    HAPPrecondition(numAdvertisingBytes);
    HAPPrecondition(!numScanResponseBytes || scanResponseBytes);

    HAPLog(&logObject, __func__);

    // CoreBluetooth automatically prepends 3 bytes for Flags to our advertisement data
    // (It adds flag 0x06: LE General Discoverable Mode bit + BR/EDR Not Supported bit)
    HAPAssert(numAdvertisingBytes >= 3);
    advertisingBytes = (void*) ((uint8_t*) advertisingBytes + 3);
    numAdvertisingBytes -= 3;

    // numScanResponseBytes may be 0
    // See HomeKit Accessory Protocol Specification R17
    // Section 7.4.2.2 HAP BLE Encrypted Notification Advertisement Format
    if (numScanResponseBytes) {
        HAPAssert(numScanResponseBytes >= 2);
        scanResponseBytes = (void*) ((uint8_t*) scanResponseBytes + 2);
        numScanResponseBytes -= 2;
    }
    NSData* advertisingData = [NSData dataWithBytes:advertisingBytes length:numAdvertisingBytes];
    NSData* scanResponse = [NSData dataWithBytes:scanResponseBytes length:numScanResponseBytes];
    [peripheral startAdvertising:advertisingInterval advertisingData:advertisingData scanResponse:scanResponse];
}

void HAPPlatformBLEPeripheralManagerStopAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager_) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;

    HAPLog(&logObject, __func__);

    [peripheral stopAdvertising];
}

void HAPPlatformBLEPeripheralManagerCancelCentralConnection(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle HAP_UNUSED) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;

    HAPLog(&logObject, __func__);

    [peripheral updateCentralConnection:nil];
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerSendHandleValueIndication(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle,
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
        const void* _Nullable bytes,
        size_t numBytes) {
    HAPPrecondition(blePeripheralManager_);
    HAPPlatformBLEPeripheralManagerInternal* blePeripheralManager =
            (HAPPlatformBLEPeripheralManagerInternal*) blePeripheralManager_;
    HAPBLEPeripheralDarwin* peripheral = (__bridge HAPBLEPeripheralDarwin*) blePeripheralManager->obj;

    HAPLog(&logObject, __func__);

    bool updateSent = [peripheral updateCharacteristic:valueHandle
                                             withValue:[NSData dataWithBytes:bytes length:numBytes]
                                         forConnection:connectionHandle];

    return (updateSent == YES) ? kHAPError_None : kHAPError_InvalidState;
}
