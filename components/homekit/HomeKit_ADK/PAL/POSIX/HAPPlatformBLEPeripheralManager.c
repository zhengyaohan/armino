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

// Note that the BLE implementation for Raspberry Pi isn't compliant with the HAP specification.
// Use this implementation only for a development reference point.

#include "HAPPlatform.h"

#if HAP_FEATURE_ENABLED(HAP_FEATURE_BLE) || HAP_FEATURE_ENABLED(HAP_FEATURE_PLATFORM_BLE)

#include "HAPPlatformBLE+BlueZ.h"
#include "HAPPlatformBLEPeripheralManager+Init.h"

#include <errno.h>
#include <gio/gio.h>
#include <glib/gprintf.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "BLEPeripheralManager" };

/**
 * Maximum connection handle value.
 *
 * This is per @ref HAPPlatformBLEPeripheralManagerConnectionHandle definition.
 */
#define kMaxConnectionHandleValue 0xEFF

/**
 * Maximum number of HCI devices supported by this module
 *
 * 2 is an arbitrary pick.
 */
#define kMaxNumHCIDevices 2

/**
 * Maximum object path length in bytes including null termination
 *
 * Note that this value has to correspond to the path naming convention used here.
 */
#define kMaxObjectPathLength 128

/**
 * DBUS bluez GATT application object root path
 */
#define kBluezAppObjectRootPath "/org/bluez/apps"

/**
 * Bluez DBUS bus name
 */
#define kBluezBus "org.bluez"

/**
 * DBus ObjectManager interface
 */
#define kDBusObjectManagerInterface "org.freedesktop.DBus.ObjectManager"

/**
 * DBus ObjectManager interface GetManagedObjects method
 */
#define kDBusObjectManagerGetManagedObjectsMethod "GetManagedObjects"

/**
 * DBus Properties interface
 */
#define kDBusPropertiesInterface "org.freedesktop.DBus.Properties"

/**
 * DBus Properties interface Get method
 */
#define kDBusPropertiesGetMethod "Get"

/**
 * DBus Properties interface PropertiesChanged signal
 */
#define kBluezPropertisChangedSignal "PropertiesChanged"

/**
 * bluez Device1 interface
 */
#define kBluezDeviceInterface "org.bluez.Device1"

/**
 * bluez Device1 interface Disconnect method
 */
#define kBluezDeviceDisconnectMethod "Disconnect"

/**
 * bluez Device1 interface Connected property
 */
#define kBluezDeviceConnectedProperty "Connected"

/**
 * bluez GattDescriptor1 interface
 */
#define kBluezGattDescriptorInterface "org.bluez.GattDescriptor1"

/**
 * bluez GattDescriptor1 interface ReadValue method
 */
#define kBluezGattDescriptorReadValueMethod "ReadValue"

/**
 * bluez GattDescriptor1 interface WriteValue method
 */
#define kBluezGattDescriptorWriteValueMethod "WriteValue"

/**
 * bluez GattDescriptor1 interface UUID property
 */
#define kBluezGattDescriptorUUIDProperty "UUID"

/**
 * bluez GattDescriptor1 interface Characteristic property
 */
#define kBluezGattDescriptorCharacteristicProperty "Characteristic"

/**
 * bluez GattDescriptor1 interface Flags property
 */
#define kBluezGattDescriptorFlagsProperty "Flags"

/**
 * bluez GattDescriptor1 interface Flags property read value
 */
#define kBluezGattDescriptorFlagsRead "read"

/**
 * bluez GattDescriptor1 interface Flags property write value
 */
#define kBluezGattDescriptorFlagsWrite "write"

/**
 * bluez GattCharacteristic1 interface
 */
#define kBluezGattCharacteristicInterface "org.bluez.GattCharacteristic1"

/**
 * bluez GattCharacteristic1 interface ReadValue method
 */
#define kBluezGattCharacteristicReadValueMethod "ReadValue"

/**
 * bluez GattCharacteristic1 interface WriteValue method
 */
#define kBluezGattCharacteristicWriteValueMethod "WriteValue"

/**
 * bluez GattCharacteristic1 interface StartNotify method
 */
#define kBluezGattCharacteristicStartNotifyMethod "StartNotify"

/**
 * bluez GattCharacteristic1 interface StopNotify method
 */
#define kBluezGattCharacteristicStopNotifyMethod "StopNotify"

/**
 * bluez GattCharacteristic1 interface UUID property
 */
#define kBluezGattCharacteristicUUIDProperty "UUID"

/**
 * bluez GattCharacteristic1 interface Service property
 */
#define kBluezGattCharacteristicServiceProperty "Service"

/**
 * bluez GattCharacteristic1 interface Descriptors property
 */
#define kBluezGattCharacteristicDescriptorsProperty "Descriptors"

/**
 * bluez GattCharacteristic1 interface Flags property
 */
#define kBluezGattCharacteristicFlagsProperty "Flags"

/**
 * bluez GattCharacteristic1 interface Flags property read value
 */
#define kBluezGattCharacteristicFlagsRead "read"

/**
 * bluez GattCharacteristic1 interface Flags property write-without-response value
 */
#define kBluezGattCharacteristicFlagsWriteWithoutResponse "write-without-response"

/**
 * bluez GattCharacteristic1 interface Flags property write value
 */
#define kBluezGattCharacteristicFlagsWrite "write"

/**
 * bluez GattCharacteristic1 interface Flags property notify value
 */
#define kBluezGattCharacteristicFlagsNotify "notify"

/**
 * bluez GattCharacteristic1 interface Flags property indicate value
 */
#define kBluezGattCharacteristicFlagsIndicate "indicate"

/**
 * bluez GattCharacteristic1 interface Value property
 */
#define kBluezGattCharacteristicValueProperty "Value"

/**
 * bluez GattService1 interface
 */
#define kBluezGattServiceInterface "org.bluez.GattService1"

/**
 * bluez GattService1 interface UUID property
 */
#define kBluezGattServiceUUIDProperty "UUID"

/**
 * bluez GattService1 interface Primary property
 */
#define kBluezGattServicePrimaryProperty "Primary"

/**
 * bluez GattService1 interface Characteristics property
 */
#define kBluezGattServiceCharacteristicsProperty "Characteristics"

/**
 * bluez LEAdvertisement1 interface
 */
#define kBluezLEAdvertisementInterface "org.bluez.LEAdvertisement1"

/**
 * bluez LEAdvertisement1 interface Release method
 */
#define kBluezLEAdvertisementReleaseMethod "Release"

/**
 * bluez LEAdvertisement1 interface Type property
 */
#define kBluezLEAdvertisementTypeProperty "Type"

/**
 * bluez LEAdvertisement1 interface Type property peripheral value
 */
#define kBluezLEAdvertisementTypePeripheral "peripheral"

/**
 * bluez LEAdvertisement1 interface Type property broadcast value
 */
#define kBluezLEAdvertisementTypeBroadcast "broadcast"

/**
 * bluez LEAdvertisement1 interface ManufacturerData property
 */
#define kBluezLEAdvertisementManufacturerDataProperty "ManufacturerData"

/**
 * bluez LEAdvertisement1 interface ServiceUUIDs property
 */
#define kBluezLEAdvertisementServiceUUIDsProperty "ServiceUUIDs"

/**
 * bluez LEAdvertisement1 interface SolicitUUIDs property
 */
#define kBluezLEAdvertisementSolicitUUIDsProperty "SolicitUUIDs"

/**
 * bluez LEAdvertisement1 interface ServiceData property
 */
#define kBluezLEAdvertisementServiceDataProperty "ServiceData"

/**
 * bluez LEAdvertisement1 interface IncludeTxPower property
 */
#define kBluezLEAdvertisementIncludeTxPowerProperty "IncludeTxPower"

/**
 * bluez LEAdvertisement1 interface LocalName property
 */
#define kBluezLEAdvertisementLocalNameProperty "LocalName"

/**
 * bluez LEAdvertisement1 interface Duration property
 */
#define kBluezLEAdvertisementDurationProperty "Duration"

/**
 * bluez LEAdvertisement1 interface Discoverable property
 */
#define kBluezLEAdvertisementDiscoverableProperty "Discoverable"

/**
 * bluez LEAdvertisement1 interface Interval property
 *
 * Note that this is a custom property added to bluez stack.
 */
#define kBluezLEAdvertisementIntervalProperty "Interval"

/**
 * bluez Adapter1 interface
 */
#define kBluezAdapterInterface "org.bluez.Adapter1"

/**
 * bluez Adapter1 interface Powered property
 */
#define kBluezAdapterPoweredProperty "Powered"

/**
 * bluez Adapter1 interface Address property
 */
#define kBluezAdapterAddressProperty "Address"

/**
 * bluez Adapter1 interface AddressType property
 */
#define kBluezAdapterAddressTypeProperty "AddressType"

/**
 * bluez GattManager1 interface
 */
#define kBluezGattManagerInterface "org.bluez.GattManager1"

/**
 * bluez GattManager1 interface RegisterApplication method
 */
#define kBluezGattManagerRegisterApplicationMethod "RegisterApplication"

/**
 * bluez GattManager1 interface UnregisterApplication method
 */
#define kBluezGattManagerUnregisterApplicationMethod "UnregisterApplication"

/**
 * bluez LEAdvertisingManager1 interface
 */
#define kBluezLEAdvertisingManagerInterface "org.bluez.LEAdvertisingManager1"

/**
 * bluez LEAdvertisingManager1 interface RegisterAdvertisement method
 */
#define kBluezLEAdvertisingManagerRegisterAdvertisementMethod "RegisterAdvertisement"

/**
 * bluez LEAdvertisingManager1 interface UnregisterAdvertisement method
 */
#define kBluezLEAdvertisingManagerUnregisterAdvertisementMethod "UnregisterAdvertisement"

/**
 * bluez NotPermitted error
 */
#define kBluezNotPermittedError "org.bluez.Error.NotPermitted"

/**
 * bluez InvalidOffset error
 */
#define kBluezInvalidOffsetError "org.bluez.Error.InvalidOffset"

/**
 * bluez Failed error
 */
#define kBluezFailedError "org.bluez.Error.Failed"

/**
 * bluez GATT_INVALID_PDU error message
 */
#define kBluezGattInvalidPduErrorMessage "0x04"

/**
 * bluez GATT_NO_RESOURCES error message
 */
#define kBluezGattNoResourcesErrorMessage "0x80"

/**
 * bluez GATT_INTERNAL_ERROR error message
 */
#define kBluezGattInternalErrorMessage "0x81"

/** Resources specific to a HCI device */
typedef struct {
    char* hciPath;            /**< hci device path */
    size_t refCount;          /**< reference counter */
    int nextServiceRootEumId; /**< enumeration id for next service root to add */
} HCIResources;

static HCIResources hciResources[kMaxNumHCIDevices];

struct gatt_service;
struct gatt_characteristic;

/** GATT characteristic descriptor object state */
typedef struct gatt_descriptor {
    struct gatt_descriptor* next;
    struct gatt_characteristic* characteristic;
    struct ble_peripheral_manager* blePeripheralManager;
    const HAPPlatformBLEPeripheralManagerUUID* uuid;
    const void* _Nullable constBytes;
    size_t constNumBytes;
    guint registrationId;
    int enumId;
    HAPPlatformBLEPeripheralManagerAttributeHandle descriptorHandle;
    HAPPlatformBLEPeripheralManagerDescriptorProperties properties;
} GATTDescriptor;

/** GATT characteristic object state */
typedef struct gatt_characteristic {
    struct gatt_characteristic* next;
    struct gatt_service* service;
    struct ble_peripheral_manager* blePeripheralManager;
    GATTDescriptor* descriptors;
    const HAPPlatformBLEPeripheralManagerUUID* uuid;
    const void* _Nullable constBytes;
    size_t constNumBytes;
    guint registrationId;
    int enumId;
    int nextDescriptorEnumId;
    HAPPlatformBLEPeripheralManagerAttributeHandle valueAttributeHandle;
    HAPPlatformBLEPeripheralManagerAttributeHandle cccDescriptorAttributeHandle;
    HAPPlatformBLEPeripheralManagerCharacteristicProperties properties;
} GATTCharacteristic;

/** GATT service object state */
typedef struct gatt_service {
    struct gatt_service* next;
    struct ble_peripheral_manager* blePeripheralManager;
    GATTCharacteristic* characteristics;
    const HAPPlatformBLEPeripheralManagerUUID* uuid;
    guint registrationId;
    int enumId;
    int nextCharacteristicEnumId;
    bool isPrimary;
} GATTService;

/** Manufacturing data in advertisement */
typedef struct advertisement_mfg_data {
    struct advertisement_mfg_data* next;
    uint16_t companyId;
    uint8_t numBytes;
    // actual bytes follow
} AdvertisementMfgData;

/** Central Connection */
typedef struct central_connection {
    struct central_connection* next;
    /** device is reported as connected to HAP */
    bool isReportedAsConnected;
    /** dbus object path */
    char* objPath;
    /** connection handle */
    HAPPlatformBLEPeripheralManagerConnectionHandle handle;
    /** cached last read value */
    struct {
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle; /** characteristic value handle */
        uint8_t* bytes;                                             /** value */
        uint16_t numBytes;                                          /** number of bytes */
    } cachedReadValue;
} CentralConnection;

/** DBUS object thread that needs to sync with runloop */
typedef struct dbus_object_thread {
    struct ble_peripheral_manager* blePeripheralManager; /**< ble peripheral manager */
    GMainContext* mainContext;                           /**< main context */
    GMainLoop* mainLoop;                                 /**< main loop */
    GDBusConnection* dbusConnection;                     /**< dbus connection */
    GThread* thread;                                     /**< thread */
    sem_t runloopWaitSemaphore;                          /**< semaphore to signal that runloop is ready to be blocked */
    sem_t runloopUnblockSemaphore;                       /**< semaphore to signal runloop to unblock and continue */
} DBusObjectThread;

/** Peripheral manager internal data structure*/
typedef struct ble_peripheral_manager {
    /** whether GATT application is registered */
    bool isGATTAppRegistered;
    /** index to the hciResources table */
    int hciResourceIndex;
    /** HAP defined BLE peripheral manager pointer */
    HAPPlatformBLEPeripheralManager* hapBLEPeripheralManager;
    /** HAP peripheral manager delegate */
    HAPPlatformBLEPeripheralManagerDelegate delegate;
    /** thread for receiving DBUS signals */
    DBusObjectThread signalReceiverThread;
    /** thread for serving DBUS GATT application, service, characteristic and descriptor objects */
    DBusObjectThread gattObjectThread;
    /** thread for serving DBUS advertisement objects */
    DBusObjectThread advertisementThread;
    /** semaphore used to sync caller thread with object thread */
    sem_t syncSemaphore;
    /** syncSemaphore is initialized */
    bool isSyncSemaphoreValid;
    /** DBus connection used in the caller (HAP runloop) context */
    GDBusConnection* dbusConnection;
    /** registration id of the GATT application object */
    guint gattApplicationRegistrationId;
    /** bluez stack interface changed event subscription id */
    guint bluezSubscriptionId;
    /** GATT service root enumeration ID */
    int serviceRootEnumId;
    /** registered services */
    GATTService* services;
    /** central connections */
    CentralConnection* centralConnections;
    /** enumeration id for next service to add */
    int nextServiceEnumId;
    /**
     * next fake handle value to use
     *
     * Since there is no interface to obtain the actual attribute handle value for service and characteristics,
     * fake values are assigned and used for interface with HAP BLE peripheral manager delegate.
     */
    HAPPlatformBLEPeripheralManagerAttributeHandle nextFakeAttributeHandle;
    /**
     * Advertisement state
     */
    struct {
        bool isAdvertising : 1;           /**< currently advertising */
        bool isPeripheral : 1;            /**< advertisement object represents a connectable peripheral */
        bool isDiscoverableOverriden : 1; /**< general discoverable value is overridden by the caller */
        bool isDiscoverable : 1;          /**< advertise as general discoverable */
        uint16_t interval;                /**< advertising interval */
        guint registrationId;             /**< DBUS advertisement object registration Id */
        char* localName;                  /**< local name */
        AdvertisementMfgData* mfgData;    /**< list of manufacturer data */
    } advertisement;
} BLEPeripheralManager;

/**
 * DBUS object thread main entry function
 */
static gpointer DBusObjectThreadMain(gpointer userData) {
    HAPPrecondition(userData);
    DBusObjectThread* thread = (DBusObjectThread*) userData;
    GMainContext* mainContext = g_main_context_ref(thread->mainContext);
    GMainLoop* mainLoop = g_main_loop_ref(thread->mainLoop);

    g_main_context_push_thread_default(mainContext);
    g_main_loop_run(mainLoop);

    g_main_loop_unref(mainLoop);
    g_main_context_pop_thread_default(mainContext);
    g_main_context_unref(mainContext);

    return NULL;
}

/**
 * Allocate a hciResources entry for the hci device path.
 *
 * @param      hciPath  HCI device path
 * @param[out] index    index to the allocated hciResources array element.
 * @return kHAPError_None when successful. kHAPError_OutOfResources when the hciResources for the hci path cannot be
 * allocated.
 */
static HAPError AllocateHCIResource(const char* hciPath, int* index) {
    int unused = -1;
    for (size_t i = 0; i < HAPArrayCount(hciResources); i++) {
        if (hciResources[i].hciPath) {
            if (HAPStringAreEqual(hciPath, hciResources[i].hciPath)) {
                *index = (size_t) i;
                hciResources[i].refCount++;
                return kHAPError_None;
            }
        } else if (unused < 0) {
            unused = i;
        }
    }
    if (unused < 0) {
        return kHAPError_OutOfResources;
    }
    HAPAssert(hciResources[unused].hciPath == NULL);
    hciResources[unused].hciPath = strdup(hciPath);
    if (!hciResources[unused].hciPath) {
        HAPLogError(&logObject, "%s: strup() failed", __func__);
        return kHAPError_OutOfResources;
    }
    hciResources[unused].refCount = 1;
    hciResources[unused].nextServiceRootEumId = 0;
    *index = unused;
    return kHAPError_None;
}

/**
 * Deallocate hciReources entry
 */
static void DeallocateHCIResource(size_t index) {
    HAPAssert(hciResources[index].refCount > 0);
    if (--hciResources[index].refCount == 0) {
        free(hciResources[index].hciPath);
        hciResources[index].hciPath = NULL;
    }
}

/**
 * Get the DBUS GATT characteristic object corresponding to the value attribute handle
 *
 * @param blePeripheralManager  ble peripheral manager object
 * @param valueHandle           value attribute handle
 * @return GATT characteristic object pointer or NULL if not found.
 */
static GATTCharacteristic* FindCharacteristicOfValueHandle(
        BLEPeripheralManager* blePeripheralManager,
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle) {
    HAPPrecondition(blePeripheralManager);

    GATTService* service = blePeripheralManager->services;
    while (service) {
        GATTCharacteristic* characteristic = service->characteristics;
        while (characteristic) {
            if (characteristic->valueAttributeHandle == valueHandle) {
                return characteristic;
            }
            characteristic = characteristic->next;
        }
        service = service->next;
    }
    return NULL;
}

/** Synchronous function */
typedef void (*SyncFunction)(void* arg);

/** Synchronous function wrapper argument data structure */
typedef struct {
    sem_t* syncSemaphore;
    SyncFunction func;
    gpointer data;
} SyncFunctionWrapperData;

/**
 * Synchronous function wrapper
 */
gboolean SyncFunctionWrapper(gpointer data) {
    HAPPrecondition(data);
    SyncFunctionWrapperData* wrapperData = (SyncFunctionWrapperData*) data;
    wrapperData->func(wrapperData->data);
    return G_SOURCE_REMOVE;
}

/**
 * Callback function when synchronous function is done with data
 */
static void HandleSyncFunctionDataFree(gpointer data) {
    HAPPrecondition(data);
    SyncFunctionWrapperData* wrapperData = (SyncFunctionWrapperData*) data;
    int ret = sem_post(wrapperData->syncSemaphore);
    HAPAssert(ret == 0);
}

/**
 * Call a synchronous function to execute in another GLib thread
 *
 * @param context        GLib main context used by the thread to run the function on.
 * @param syncSemaphore  semaphore to use to sync the calling thread with the function call.
 * @param func           function to call
 * @param data           data to pass to the function
 *
 * @return @ref kHAPError_None when successful. @ref kHAPError_OutOfResources when it ran out of resources.
 */
static HAPError InvokeSyncFunction(GMainContext* context, sem_t* syncSemaphore, SyncFunction func, gpointer data) {
    GSource* idleSource = g_idle_source_new();
    if (!idleSource) {
        return kHAPError_OutOfResources;
    }
    SyncFunctionWrapperData wrapperData = {
        .syncSemaphore = syncSemaphore,
        .func = func,
        .data = data,
    };
    g_source_set_callback(idleSource, SyncFunctionWrapper, &wrapperData, HandleSyncFunctionDataFree);
    g_source_set_priority(idleSource, G_PRIORITY_DEFAULT);
    g_source_attach(idleSource, context);
    g_source_unref(idleSource);
    for (;;) {
        int ret = sem_wait(syncSemaphore);
        if (ret == 0) {
            break;
        }
        HAPAssert(errno == EINTR);
    }
    return kHAPError_None;
}

/**
 * Handle runloop scheduler callback to execute the runloop sync from runloop thread
 */
void HandleRunloopSyncSchedulerCallback(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    HAPPrecondition(contextSize = sizeof(DBusObjectThread*));
    DBusObjectThread** thread = context;
    HAPPrecondition(*thread);

    int ret = sem_post(&(*thread)->runloopWaitSemaphore);
    HAPAssert(ret == 0);

    for (;;) {
        int ret = sem_wait(&(*thread)->runloopUnblockSemaphore);
        if (ret == 0) {
            break;
        }
        HAPAssert(errno == EINTR);
    }
}

/**
 * Begin a region of code synchronized with runloop thread
 */
static void BeginRunloopSync(DBusObjectThread* thread) {
    HAPPrecondition(thread);

    HAPError err = HAPPlatformRunLoopScheduleCallback(HandleRunloopSyncSchedulerCallback, &thread, sizeof thread);
    if (err) {
        // Cannot move on
        HAPFatalError();
    }
    for (;;) {
        int ret = sem_wait(&thread->runloopWaitSemaphore);
        if (ret == 0) {
            break;
        }
        HAPAssert(errno == EINTR);
    }
}

/**
 * Begin the region of code synchronized with runloop thread
 */
static void EndRunloopSync(DBusObjectThread* thread) {
    HAPPrecondition(thread);

    int ret = sem_post(&thread->runloopUnblockSemaphore);
    HAPAssert(ret == 0);
}

/** argument for GetSystemDBusConnection and UnrefDBusConnection */
typedef struct {
    GDBusConnection* connection;
} DBusConnectionArgs;

/** Get a system DBUS connection */
static void GetSystemDBusConnection(gpointer data) {
    HAPPrecondition(data);
    DBusConnectionArgs* args = (DBusConnectionArgs*) data;

    GError* error = NULL;
    args->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error) {
        HAPLogError(&logObject, "%s: %s", __func__, error->message);
        g_error_free(error);
        args->connection = NULL;
    }
}

/** Unref DBUS connection */
static void UnrefDBusConnection(gpointer data) {
    HAPPrecondition(data);
    DBusConnectionArgs* args = (DBusConnectionArgs*) data;

    g_object_unref(args->connection);
}

/** Stop thread */
static void StopThread(gpointer data) {
    HAPPrecondition(data);
    DBusObjectThread* thread = (DBusObjectThread*) data;
    HAPPrecondition(thread->mainLoop);

    g_main_loop_quit(thread->mainLoop);
}

/**
 * Destroy a DBus object thread
 *
 * @param thread                thread object. Semaphores must have been already initialized.
 *                              blePeripheralManager field must also be initialized with
 *                              syncSemaphore initialized.
 */
static void DestroyDBusObjectThread(DBusObjectThread* thread) {
    HAPPrecondition(thread);
    HAPPrecondition(thread->blePeripheralManager);

    if (thread->dbusConnection) {
        HAPAssert(thread->thread);
        HAPAssert(thread->mainContext);
        DBusConnectionArgs args = { .connection = thread->dbusConnection };
        HAPError err = InvokeSyncFunction(
                thread->mainContext, &thread->blePeripheralManager->syncSemaphore, UnrefDBusConnection, &args);
        if (err) {
            HAPFatalError();
        }
    }
    if (thread->thread) {
        HAPPrecondition(thread->mainLoop);
        HAPPrecondition(thread->mainContext);
        HAPError err = InvokeSyncFunction(
                thread->mainContext, &thread->blePeripheralManager->syncSemaphore, StopThread, thread);
        if (err) {
            HAPFatalError();
        }
        g_thread_join(thread->thread);
        thread->thread = NULL;
    }
    if (thread->mainLoop) {
        g_main_loop_unref(thread->mainLoop);
        thread->mainLoop = NULL;
    }
    if (thread->mainContext) {
        g_main_context_unref(thread->mainContext);
        thread->mainContext = NULL;
    }
    sem_destroy(&thread->runloopWaitSemaphore);
    sem_destroy(&thread->runloopUnblockSemaphore);
}

/** Launch a DBus Object thread and initialize @ref DBusObjectThread object */
static HAPError
        LaunchDBusObjectThread(DBusObjectThread* thread, char* threadName, BLEPeripheralManager* blePeripheralManager) {
    HAPPrecondition(thread);
    HAPPrecondition(blePeripheralManager);

    int ret = sem_init(&thread->runloopWaitSemaphore, 0, 0);
    if (ret) {
        return kHAPError_Unknown;
    }

    ret = sem_init(&thread->runloopUnblockSemaphore, 0, 0);
    if (ret) {
        sem_destroy(&thread->runloopWaitSemaphore);
        return kHAPError_Unknown;
    }

    thread->blePeripheralManager = blePeripheralManager;

    thread->mainContext = g_main_context_new();
    if (!thread->mainContext) {
        sem_destroy(&thread->runloopWaitSemaphore);
        sem_destroy(&thread->runloopUnblockSemaphore);
        return kHAPError_OutOfResources;
    }
    thread->mainLoop = g_main_loop_new(thread->mainContext, FALSE);
    if (!thread->mainLoop) {
        DestroyDBusObjectThread(thread);
        return kHAPError_OutOfResources;
    }

    thread->thread = g_thread_new(threadName, DBusObjectThreadMain, thread);
    if (!thread->thread) {
        HAPLogError(&logObject, "%s: Failed to create a thread %s", __func__, threadName);
        DestroyDBusObjectThread(thread);
        return kHAPError_OutOfResources;
    }
    DBusConnectionArgs args = { .connection = NULL };
    HAPError err = InvokeSyncFunction(
            thread->mainContext, &blePeripheralManager->syncSemaphore, GetSystemDBusConnection, &args);
    if (err) {
        DestroyDBusObjectThread(thread);
    }
    thread->dbusConnection = args.connection;

    return kHAPError_None;
}

/** Free central connections of BLEPeripheralManager object */
static void FreeBLEPeripheralManagerCentralConnections(BLEPeripheralManager* blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    CentralConnection* connection = blePeripheralManager->centralConnections;
    while (connection) {
        free(connection->objPath);
        CentralConnection* next = connection->next;
        if (connection->cachedReadValue.bytes) {
            free(connection->cachedReadValue.bytes);
        }
        free(connection);
        connection = next;
    }
    blePeripheralManager->centralConnections = NULL;
}

/** Free advertisement data of BLEPeripheralManager object */
static void FreeBLEPeripheralManagerAdvertisement(BLEPeripheralManager* blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    if (blePeripheralManager->advertisement.localName) {
        free(blePeripheralManager->advertisement.localName);
        blePeripheralManager->advertisement.localName = NULL;
    }

    AdvertisementMfgData* mfgData = blePeripheralManager->advertisement.mfgData;
    while (mfgData) {
        AdvertisementMfgData* next = mfgData->next;
        free(mfgData);
        mfgData = next;
    }
    blePeripheralManager->advertisement.mfgData = NULL;
    blePeripheralManager->advertisement.isDiscoverableOverriden = false;
}

/** Free a BLEPeripheralManager object */
static void FreeBLEPeripheralManager(BLEPeripheralManager* blePeripheralManager) {
    if (blePeripheralManager->hciResourceIndex >= 0) {
        DeallocateHCIResource(blePeripheralManager->hciResourceIndex);
    }
    if (blePeripheralManager->signalReceiverThread.mainContext) {
        DestroyDBusObjectThread(&blePeripheralManager->signalReceiverThread);
    }
    if (blePeripheralManager->gattObjectThread.mainContext) {
        DestroyDBusObjectThread(&blePeripheralManager->gattObjectThread);
    }
    if (blePeripheralManager->advertisementThread.mainContext) {
        DestroyDBusObjectThread(&blePeripheralManager->advertisementThread);
    }
    if (blePeripheralManager->isSyncSemaphoreValid) {
        sem_destroy(&blePeripheralManager->syncSemaphore);
    }
    if (blePeripheralManager->dbusConnection) {
        g_object_unref(blePeripheralManager->dbusConnection);
    }
    FreeBLEPeripheralManagerCentralConnections(blePeripheralManager);
    FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
    free(blePeripheralManager);
}

/**
 * Find or add a central connection entry to the BLEPeripheralManager
 *
 * @param blePeripheralManager     ble peripheral manager
 * @param objPath                  DBUS object path to the central device. The string can be freed after the call.
 *
 * @return connection or NULL if ran out of memory.
 */
static CentralConnection*
        FindBLEPeripheralManagerCentralConnection(BLEPeripheralManager* blePeripheralManager, const char* objPath) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(objPath);
    HAPLogDebug(&logObject, "%s: %s", __func__, objPath);

    // Initialize non-overlapping connection handle value for a new connection
    HAPPlatformBLEPeripheralManagerConnectionHandle newConnectionHandle = 0;

    CentralConnection* connection = blePeripheralManager->centralConnections;
    while (connection) {
        if (g_strcmp0(objPath, connection->objPath) == 0) {
            // Device found
            return connection;
        }
        newConnectionHandle = HAPMax(newConnectionHandle, connection->handle + 1);
        connection = connection->next;
    }

    if (newConnectionHandle > kMaxConnectionHandleValue) {
        // Find a new value
        for (newConnectionHandle = 0; newConnectionHandle <= kMaxConnectionHandleValue; newConnectionHandle++) {
            connection = blePeripheralManager->centralConnections;
            bool isUsed = false;
            while (connection) {
                if (newConnectionHandle == connection->handle) {
                    isUsed = true;
                    break;
                }
                connection = connection->next;
            }
            if (!isUsed) {
                break;
            }
        }
    }
    if (newConnectionHandle > kMaxConnectionHandleValue) {
        HAPLogError(&logObject, "%s: connections full", __func__);
        return NULL;
    }

    // Device was not found. Add a new entry.
    connection = malloc(sizeof(CentralConnection));
    if (!connection) {
        HAPLogError(&logObject, "%s: malloc failed", __func__);
        return NULL;
    }
    HAPRawBufferZero(connection, sizeof *connection);
    connection->objPath = strdup(objPath);
    if (!connection->objPath) {
        HAPLogError(&logObject, "%s: strdup failed", __func__);
        free(connection);
        return NULL;
    }
    connection->handle = newConnectionHandle;
    connection->next = blePeripheralManager->centralConnections;
    blePeripheralManager->centralConnections = connection;

    return connection;
}

/**
 * Find a central connection entry from the connection handle
 *
 * @param blePeripheralManager     ble peripheral manager
 * @param handle                   connection handle
 *
 * @return connection or NULL if connection entry is not found.
 */
static CentralConnection* FindBLEPeripheralManagerCentralConnectionFromHandle(
        BLEPeripheralManager* blePeripheralManager,
        HAPPlatformBLEPeripheralManagerConnectionHandle handle) {
    HAPPrecondition(blePeripheralManager);

    CentralConnection* connection = blePeripheralManager->centralConnections;
    while (connection) {
        if (connection->handle == handle) {
            break;
        }
        connection = connection->next;
    }
    return connection;
}

/**
 * Drop a central connection entry of the BLEPeripheralManager
 *
 * @param blePeripheralManager     ble peripheral manager
 * @param objPath                  DBUS object path to the central device. The string can be freed after the call.
 */
static void DropBLEPeripheralManagerCentralConnection(BLEPeripheralManager* blePeripheralManager, const char* objPath) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(objPath);
    HAPLogDebug(&logObject, "%s: %s", __func__, objPath);

    CentralConnection** connectionPtr = &blePeripheralManager->centralConnections;
    while (*connectionPtr) {
        if (g_strcmp0(objPath, (*connectionPtr)->objPath) == 0) {
            // Device found
            break;
        }
        connectionPtr = &(*connectionPtr)->next;
    }

    if (*connectionPtr) {
        CentralConnection* connection = *connectionPtr;
        free(connection->objPath);
        if (connection->cachedReadValue.bytes) {
            free(connection->cachedReadValue.bytes);
        }
        *connectionPtr = connection->next;
        free(connection);
    }
}

/** Buffer size required for full length UUID string including null-termination */
#define kUUIDStringBufferSize 37

/**
 * Convert UUID into a string
 *
 * @param[in] uuid     UUID
 * @param[out] bytes   buffer to store the converted string
 * @param[in] maxBytes number of bytes of the buffer space. This should be at least @ref kUUIDStringBufferSize.
 *
 * @return buffer pointer
 */
static char* ConvertUUIDToString(const HAPPlatformBLEPeripheralManagerUUID* uuid, char* bytes, size_t maxBytes) {
    HAPError err = HAPStringWithFormat(
            bytes,
            maxBytes,
            "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            uuid->bytes[15],
            uuid->bytes[14],
            uuid->bytes[13],
            uuid->bytes[12],
            uuid->bytes[11],
            uuid->bytes[10],
            uuid->bytes[9],
            uuid->bytes[8],
            uuid->bytes[7],
            uuid->bytes[6],
            uuid->bytes[5],
            uuid->bytes[4],
            uuid->bytes[3],
            uuid->bytes[2],
            uuid->bytes[1],
            uuid->bytes[0]);
    HAPAssert(!err);
    return bytes;
}

/**
 * Build per BLE peripheral manager object path root
 */
static HAPError
        BuildGATTServiceObjectRootPath(BLEPeripheralManager* blePeripheralManager, char* bytes, size_t maxBytes) {
    return HAPStringWithFormat(
            bytes,
            maxBytes,
            kBluezAppObjectRootPath "/hci%d/services%d",
            blePeripheralManager->hciResourceIndex,
            blePeripheralManager->serviceRootEnumId);
}

/**
 * Build service object path
 */
static HAPError BuildGATTServiceObjectPath(GATTService* service, char* bytes, size_t maxBytes) {
    HAPError err = BuildGATTServiceObjectRootPath(service->blePeripheralManager, bytes, maxBytes);
    if (err) {
        return err;
    }
    size_t ret = HAPStringGetNumBytes(bytes);
    return HAPStringWithFormat(&bytes[ret], maxBytes - (size_t) ret, "/service%d", service->enumId);
}

/**
 * Build characteristic object path
 */
static HAPError BuildGATTCharacteristicObjectPath(GATTCharacteristic* characteristic, char* bytes, size_t maxBytes) {
    HAPError err = BuildGATTServiceObjectPath(characteristic->service, bytes, maxBytes);
    if (err) {
        return err;
    }
    size_t ret = HAPStringGetNumBytes(bytes);
    return HAPStringWithFormat(&bytes[ret], maxBytes - (size_t) ret, "/char%d", characteristic->enumId);
}

/**
 * Build DBUS GATT characteristic descriptor object path
 */
static HAPError BuildGATTDescriptorObjectPath(GATTDescriptor* descriptor, char* bytes, size_t maxBytes) {
    HAPError err = BuildGATTCharacteristicObjectPath(descriptor->characteristic, bytes, maxBytes);
    if (err) {
        return err;
    }
    size_t ret = HAPStringGetNumBytes(bytes);
    return HAPStringWithFormat(&bytes[ret], maxBytes - (size_t) ret, "/desc%d", descriptor->enumId);
}

/**
 * Build DBUS advertisement object path
 */
static HAPError BuildAdvertisementObjectPath(BLEPeripheralManager* blePeripheralManager, char* bytes, size_t maxBytes) {
    return HAPStringWithFormat(
            bytes,
            maxBytes,
            kBluezAppObjectRootPath "/hci%d/adv%d",
            blePeripheralManager->hciResourceIndex,
            blePeripheralManager->serviceRootEnumId);
}

/**
 * Get central connection from "device" option and offset from "offset" option among options
 *
 * @param      blePeripheralManager  BLE peripheral manager
 * @param      iter                  variant iterator for the options
 * @param[out] offset                offset if present or zero
 *
 * @return connection or NULL if device option is absent or if accessory ran out of memory.
 */
static CentralConnection* GetConnectionAndOffsetFromDeviceOption(
        BLEPeripheralManager* blePeripheralManager,
        GVariantIter* iter,
        uint16_t* offset) {
    char* key;
    GVariant* value;
    CentralConnection* connection = NULL;
    bool isDeviceOptionPresent = false;
    *offset = 0;
    while (g_variant_iter_loop(iter, "{sv}", &key, &value)) {
        if (g_strcmp0(key, "device") == 0) {
            isDeviceOptionPresent = true;
            char* path;
            g_variant_get(value, "o", &path);
            connection = FindBLEPeripheralManagerCentralConnection(blePeripheralManager, path);
            g_free(path);
        } else if (g_strcmp0(key, "offset") == 0) {
            g_variant_get(value, "q", offset);
        }
    }
    if (!isDeviceOptionPresent) {
        HAPLogError(&logObject, "%s: device option is not found", __func__);
    }
    return connection;
}

/**
 * Get central connection from "device" option among options
 *
 * @param blePeripheralManager  BLE peripheral manager
 * @param iter                  variant iterator for the options
 *
 * @return connection or NULL if device option is absent or if accessory ran out of memory.
 */
static CentralConnection*
        GetConnectionFromDeviceOption(BLEPeripheralManager* blePeripheralManager, GVariantIter* iter) {
    char* key;
    GVariant* value;
    CentralConnection* connection = NULL;
    bool isDeviceOptionPresent = false;
    while (g_variant_iter_loop(iter, "{sv}", &key, &value)) {
        if (g_strcmp0(key, "device") == 0) {
            isDeviceOptionPresent = true;
            char* path;
            g_variant_get(value, "o", &path);
            connection = FindBLEPeripheralManagerCentralConnection(blePeripheralManager, path);
            g_free(path);
        }
    }
    if (!isDeviceOptionPresent) {
        HAPLogError(&logObject, "%s: device option is not found", __func__);
    }
    return connection;
}

/**
 * Check whether central is still connected and
 * if it is, call delegate to update central state if necessary.
 * If it is not, drop the connection.
 *
 * This function must be called in sync with Runloop scheduler thread.
 *
 * @param blePeripheralManager  ble peripheral manager
 * @param dbusConnection        dbus connection with system dbus from the calling thread.
 * @param centralConnection     central connection to check. This can be null.
 * @return true if the central is still connected. False, otherwise or in error conditions.
 */
static bool IsCentralStillConnected(
        BLEPeripheralManager* blePeripheralManager,
        GDBusConnection* dbusConnection,
        CentralConnection* _Nullable centralConnection) {
    HAPPrecondition(blePeripheralManager);

    if (!centralConnection) {
        // No connection
        HAPLogDebug(&logObject, "%s: no connection to start with", __func__);
        return false;
    }

    GError* error = NULL;
    GVariant* result = g_dbus_connection_call_sync(
            dbusConnection,
            kBluezBus,
            centralConnection->objPath,
            kDBusPropertiesInterface,
            kDBusPropertiesGetMethod,
            g_variant_new("(ss)", kBluezDeviceInterface, kBluezDeviceConnectedProperty),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

    if (error) {
        HAPLogDebug(
                &logObject,
                "%s: couldn't get Connected property of %s: %s",
                __func__,
                centralConnection->objPath,
                error->message);
        g_error_free(error);
        return false;
    }

    HAPAssert(g_variant_n_children(result) == 1);
    GVariant* element = g_variant_get_child_value(result, 0);
    GVariant* elementValue = g_variant_get_variant(element);
    gboolean isConnected = false;
    g_variant_get(elementValue, "b", &isConnected);
    g_variant_unref(elementValue);
    g_variant_unref(element);
    g_variant_unref(result);

    if (isConnected) {
        if (!centralConnection->isReportedAsConnected) {
            // Connection was not reported to the peripheral delegate yet.
            // This could happen if the ADK ran after central was already connected.
            // Report it.
            if (blePeripheralManager->delegate.handleConnectedCentral) {
                blePeripheralManager->delegate.handleConnectedCentral(
                        blePeripheralManager->hapBLEPeripheralManager,
                        centralConnection->handle,
                        blePeripheralManager->delegate.context);
            }
            centralConnection->isReportedAsConnected = true;
        }
        return true;
    }

    if (centralConnection->isReportedAsConnected) {
        // Disconnection was not reported to the peripheral delegate yet.
        // This could happen as a race condition between signal receive thread and
        // GATT object thread receiving StopNotify.
        if (blePeripheralManager->delegate.handleDisconnectedCentral) {
            blePeripheralManager->delegate.handleDisconnectedCentral(
                    blePeripheralManager->hapBLEPeripheralManager,
                    centralConnection->handle,
                    blePeripheralManager->delegate.context);
        }
    }

    // Not connected. Drop the connection entry.
    DropBLEPeripheralManagerCentralConnection(blePeripheralManager, centralConnection->objPath);
    return false;
}

/**
 * Handle method call to DBUS GATT Characteristic Descriptor object
 */
static void GATTDescriptorHandleMethodCall(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender,
        const gchar* objectPath,
        const gchar* interfaceName,
        const gchar* methodName,
        GVariant* parameters,
        GDBusMethodInvocation* invocation,
        gpointer userData) {
    HAPPrecondition(userData);
    GATTDescriptor* descriptor = (GATTDescriptor*) userData;
    HAPLogDebug(&logObject, "%s: %s -> %s: %s %s", __func__, sender, objectPath, interfaceName, methodName);
    if (g_strcmp0(interfaceName, kBluezGattDescriptorInterface) == 0) {
        if (g_strcmp0(methodName, kBluezGattDescriptorReadValueMethod) == 0) {
            GVariantIter* optionIter;
            g_variant_get(parameters, "(a{sv})", &optionIter);
            BeginRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
            CentralConnection* connection = GetConnectionFromDeviceOption(descriptor->blePeripheralManager, optionIter);
            g_variant_iter_free(optionIter);
            if (!IsCentralStillConnected(
                        descriptor->blePeripheralManager,
                        descriptor->blePeripheralManager->gattObjectThread.dbusConnection,
                        connection)) {
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                HAPLogInfo(&logObject, "%s: returning error to ReadValue for lack of central connection", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
                return;
            }
            if (descriptor->constBytes) {
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
                for (size_t i = 0; i < descriptor->constNumBytes; i++) {
                    g_variant_builder_add(builder, "y", ((guchar*) descriptor->constBytes)[i]);
                }
                GVariant* ret = g_variant_builder_end(builder);
                g_variant_builder_unref(builder);
                g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&ret, 1));
            } else if (descriptor->blePeripheralManager->delegate.handleReadRequest) {
                uint8_t bytes[kHAPPlatformBLEPeripheralManager_MaxAttributeBytes];
                size_t numBytes;
                HAPError err = descriptor->blePeripheralManager->delegate.handleReadRequest(
                        descriptor->blePeripheralManager->hapBLEPeripheralManager,
                        connection->handle,
                        descriptor->descriptorHandle,
                        bytes,
                        sizeof bytes,
                        &numBytes,
                        descriptor->blePeripheralManager->delegate.context);
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
                    // This correspond to Darwin implementation but there is a note in nRF52 implementation
                    // that some iOS and HAT cannot handle GATT errors and get stuck.
                    // nRF52 implementation disconnects central in this case.
                    // 0x80 => GATT_NO_RESOURCES.
                    g_dbus_method_invocation_return_dbus_error(
                            invocation, kBluezFailedError, kBluezGattNoResourcesErrorMessage);
                } else {
                    GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
                    for (size_t i = 0; i < numBytes; i++) {
                        g_variant_builder_add(builder, "y", bytes[i]);
                    }
                    GVariant* ret = g_variant_builder_end(builder);
                    g_variant_builder_unref(builder);
                    g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&ret, 1));
                }
            } else {
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                // This correspond to Darwin implementation but there is a note in nRF52 implementation
                // that some iOS and HAT cannot handle GATT errors and get stuck.
                // nRF52 implementation disconnected central in this case.
                HAPLogError(&logObject, "%s: no delegate read handler", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
            }
        } else if (g_strcmp0(methodName, kBluezGattDescriptorWriteValueMethod) == 0) {
            GVariantIter* valueIter;
            GVariantIter* optionIter;
            g_variant_get(parameters, "(aya{sv})", &valueIter, &optionIter);
            BeginRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
            CentralConnection* connection = GetConnectionFromDeviceOption(descriptor->blePeripheralManager, optionIter);
            g_variant_iter_free(optionIter);
            if (!IsCentralStillConnected(
                        descriptor->blePeripheralManager,
                        descriptor->blePeripheralManager->gattObjectThread.dbusConnection,
                        connection)) {
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                HAPLogInfo(&logObject, "%s: returning error to WriteValue for lack of central connection", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
                return;
            }
            uint8_t byte;
            uint8_t bytes[kHAPPlatformBLEPeripheralManager_MaxAttributeBytes];
            size_t numBytes = 0;
            while (g_variant_iter_loop(valueIter, "y", &byte)) {
                if (numBytes < sizeof bytes) {
                    bytes[numBytes] = byte;
                }
                numBytes++;
            }
            if (numBytes > sizeof bytes) {
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                HAPLogError(&logObject, "%s: WriteValue with too many (%zu) bytes.", __func__, numBytes);
                g_dbus_method_invocation_return_dbus_error(
                        invocation, kBluezFailedError, kBluezGattNoResourcesErrorMessage);
            } else if (descriptor->blePeripheralManager->delegate.handleWriteRequest) {
                HAPError err = descriptor->blePeripheralManager->delegate.handleWriteRequest(
                        descriptor->blePeripheralManager->hapBLEPeripheralManager,
                        connection->handle,
                        descriptor->descriptorHandle,
                        bytes,
                        numBytes,
                        descriptor->blePeripheralManager->delegate.context);
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
                    // This correspond to Darwin implementation but there is a note in nRF52 implementation
                    // that some iOS and HAT cannot handle GATT errors and get stuck.
                    // nRF52 implementation disconnects central in this case.
                    // 0x04 => GATT_INVALID_PDU.
                    g_dbus_method_invocation_return_dbus_error(
                            invocation, kBluezFailedError, kBluezGattInvalidPduErrorMessage);
                } else {
                    g_dbus_method_invocation_return_value(invocation, NULL);
                }
            } else {
                EndRunloopSync(&descriptor->blePeripheralManager->gattObjectThread);
                HAPLogError(&logObject, "%s: no delegate write handler", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
            }
        } else {
            HAPLogError(&logObject, "%s: Unknown method call: %s", __func__, methodName);
        }
    } else {
        HAPLogError(&logObject, "%s: Unknown interface method call: %s %s", __func__, interfaceName, methodName);
    }
}

/**
 * Get a property value of a DBUS GATT Characteristic Descriptor object
 *
 * @param descriptor    DBUS GATT characteristic descriptor object
 * @param propertyName  property name
 *
 * @return GVariant object pointer corresponding to the property value
 */
static GVariant* GATTDescriptorGetProperty(GATTDescriptor* descriptor, const gchar* propertyName) {
    GVariant* ret = NULL;
    if (g_strcmp0(propertyName, kBluezGattDescriptorUUIDProperty) == 0) {
        char bytes[kUUIDStringBufferSize];
        ret = g_variant_new_string(ConvertUUIDToString(descriptor->uuid, bytes, sizeof bytes));
    } else if (g_strcmp0(propertyName, kBluezGattDescriptorCharacteristicProperty) == 0) {
        char path[kMaxObjectPathLength];
        HAPError err = BuildGATTCharacteristicObjectPath(descriptor->characteristic, path, sizeof path);
        HAPAssert(!err);
        ret = g_variant_new_object_path(path);
    } else if (g_strcmp0(propertyName, kBluezGattDescriptorFlagsProperty) == 0) {
        GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
        if (descriptor->properties.read) {
            g_variant_builder_add(builder, "s", kBluezGattDescriptorFlagsRead);
        }
        if (descriptor->properties.write) {
            g_variant_builder_add(builder, "s", kBluezGattDescriptorFlagsWrite);
        }
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else {
        g_assert(FALSE);
    }
    return ret;
}

/**
 * Get all properties of DBUS GATT Characteristic Descriptor object
 *
 * @param descriptor  descriptor
 * @param builder     variant builder to build the resulting property name and property value pairs as dictionary
 *                    entries.
 */
static void GATTDescriptorGetAllProperties(GATTDescriptor* descriptor, GVariantBuilder* builder) {
    const char* allProps[] = { kBluezGattDescriptorUUIDProperty,
                               kBluezGattDescriptorCharacteristicProperty,
                               kBluezGattDescriptorFlagsProperty };
    for (size_t i = 0; i < HAPArrayCount(allProps); i++) {
        g_variant_builder_open(builder, G_VARIANT_TYPE("{sv}"));
        g_variant_builder_add(builder, "s", allProps[i]);
        g_variant_builder_add(builder, "v", GATTDescriptorGetProperty(descriptor, allProps[i]));
        g_variant_builder_close(builder);
    }
}

/**
 * Handle GetProperty call to DBUS GATT Characteristic Descriptor object
 */
static GVariant* GATTDescriptorHandleGetProperty(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender HAP_UNUSED,
        const gchar* objectPath,
        const gchar* interfaceName,
        const gchar* propertyName,
        GError** error HAP_UNUSED,
        gpointer userData) {
    g_assert(g_strcmp0(interfaceName, kBluezGattDescriptorInterface) == 0);
    g_assert(userData != NULL);
    GATTDescriptor* descriptor = (GATTDescriptor*) userData;

    g_printf("%s: %s %s %s\n", __func__, objectPath, interfaceName, propertyName);
    return GATTDescriptorGetProperty(descriptor, propertyName);
}

/**
 * Create a DBUS GATT Characteristic object
 */
static guint CreateGATTDescriptorObject(GDBusConnection* conn, GATTDescriptor* descriptor) {
    HAPPrecondition(conn);
    HAPPrecondition(descriptor);

    const char introspectionXml[] =
            "<node>"
            "  <interface name='" kBluezGattDescriptorInterface
            "'>"
            "    <method name='" kBluezGattDescriptorReadValueMethod
            "'>"
            "      <arg name='options' type='a{sv}' direction='in'/>"
            "      <arg name='value' type='ay' direction='out'/>"
            "    </method>"
            "    <method name='" kBluezGattDescriptorWriteValueMethod
            "'>"
            "      <arg name='value' type='ay' direction='in'/>"
            "      <arg name='options' type='a{sv}' direction='in'/>"
            "    </method>"
            "    <property type='s' name='" kBluezGattDescriptorUUIDProperty
            "' access='read'/>"
            "    <property type='o' name='" kBluezGattDescriptorCharacteristicProperty
            "' access='read'/>"
            "    <property type='ay' name='" kBluezGattDescriptorFlagsProperty
            "' access='read'/>"
            "  </interface>"
            "</node>";
    GError* error = NULL;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(introspectionXml, &error);
    if (error) {
        HAPLogError(&logObject, "%s:%s", __func__, error->message);
        g_error_free(error);
        return 0;
    }

    GDBusInterfaceVTable vtable;
    vtable.method_call = GATTDescriptorHandleMethodCall;
    vtable.get_property = GATTDescriptorHandleGetProperty;
    vtable.set_property = NULL;

    char path[kMaxObjectPathLength];
    HAPError err = BuildGATTDescriptorObjectPath(descriptor, path, sizeof path);
    HAPAssert(!err);
    guint registrationId =
            g_dbus_connection_register_object(conn, path, nodeInfo->interfaces[0], &vtable, descriptor, NULL, &error);
    g_dbus_node_info_unref(nodeInfo);
    if (error) {
        HAPLogError(&logObject, "%s:%s", __func__, error->message);
        g_error_free(error);
        return 0;
    }
    return registrationId;
}

/**
 * Handle method call to DBUS GATT Characteristic object
 */
static void GATTCharacteristicHandleMethodCall(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender,
        const gchar* objectPath,
        const gchar* interfaceName,
        const gchar* methodName,
        GVariant* parameters,
        GDBusMethodInvocation* invocation,
        gpointer userData) {
    HAPPrecondition(userData);
    GATTCharacteristic* characteristic = (GATTCharacteristic*) userData;
    HAPPrecondition(characteristic->blePeripheralManager);
    HAPLogDebug(&logObject, "%s: %s -> %s: %s %s", __func__, sender, objectPath, interfaceName, methodName);
    if (g_strcmp0(interfaceName, kBluezGattCharacteristicInterface) == 0) {
        if (g_strcmp0(methodName, kBluezGattCharacteristicReadValueMethod) == 0) {
            GVariantIter* optionIter;
            g_variant_get(parameters, "(a{sv})", &optionIter);
            BeginRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
            uint16_t offset = 0;
            CentralConnection* connection =
                    GetConnectionAndOffsetFromDeviceOption(characteristic->blePeripheralManager, optionIter, &offset);
            g_variant_iter_free(optionIter);
            if (!IsCentralStillConnected(
                        characteristic->blePeripheralManager,
                        characteristic->blePeripheralManager->gattObjectThread.dbusConnection,
                        connection)) {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                HAPLogInfo(&logObject, "%s: returning error to ReadValue for lack of central connection", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
                return;
            }
            if (characteristic->constBytes) {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
                for (size_t i = offset; i < characteristic->constNumBytes; i++) {
                    g_variant_builder_add(builder, "y", ((guchar*) characteristic->constBytes)[i]);
                }
                GVariant* ret = g_variant_builder_end(builder);
                g_variant_builder_unref(builder);
                g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&ret, 1));
            } else if (characteristic->blePeripheralManager->delegate.handleReadRequest) {
                uint8_t bytes[kHAPPlatformBLEPeripheralManager_MaxAttributeBytes];
                size_t numBytes;
                HAPError err = kHAPError_None;
                if (offset > 0) {
                    HAPLogDebug(&logObject, "%s: readvalue offset=%u", __func__, (unsigned) offset);
                    if (connection->cachedReadValue.bytes &&
                        connection->cachedReadValue.valueHandle == characteristic->valueAttributeHandle) {
                        numBytes = connection->cachedReadValue.numBytes;
                        HAPRawBufferCopyBytes(bytes, connection->cachedReadValue.bytes, numBytes);
                    } else {
                        HAPLogError(&logObject, "%s: no cache for offset=%u", __func__, (unsigned) offset);
                        g_dbus_method_invocation_return_dbus_error(invocation, kBluezInvalidOffsetError, "");
                        return;
                    }
                } else {
                    if (connection->cachedReadValue.bytes) {
                        free(connection->cachedReadValue.bytes);
                        connection->cachedReadValue.bytes = NULL;
                        connection->cachedReadValue.numBytes = 0;
                    }
                    err = characteristic->blePeripheralManager->delegate.handleReadRequest(
                            characteristic->blePeripheralManager->hapBLEPeripheralManager,
                            connection->handle,
                            characteristic->valueAttributeHandle,
                            bytes,
                            sizeof bytes,
                            &numBytes,
                            characteristic->blePeripheralManager->delegate.context);
                    if (!err) {
                        connection->cachedReadValue.numBytes = numBytes;
                        connection->cachedReadValue.bytes = malloc(numBytes);
                        if (!connection->cachedReadValue.bytes) {
                            HAPLogError(
                                    &logObject,
                                    "%s: failed to malloc for readvalue cache. Future read blob may fail.",
                                    __func__);
                        } else {
                            HAPRawBufferCopyBytes(connection->cachedReadValue.bytes, bytes, numBytes);
                            connection->cachedReadValue.valueHandle = characteristic->valueAttributeHandle;
                        }
                    }
                }
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidState || err == kHAPError_OutOfResources);
                    // This correspond to Darwin implementation but there is a note in nRF52 implementation
                    // that some iOS and HAT cannot handle GATT errors and get stuck.
                    // nRF52 implementation disconnects central in this case.
                    // 0x80 => GATT_NO_RESOURCES.
                    g_dbus_method_invocation_return_dbus_error(
                            invocation, kBluezFailedError, kBluezGattNoResourcesErrorMessage);
                } else {
                    GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
                    for (size_t i = offset; i < numBytes; i++) {
                        g_variant_builder_add(builder, "y", bytes[i]);
                    }
                    GVariant* ret = g_variant_builder_end(builder);
                    g_variant_builder_unref(builder);
                    g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&ret, 1));
                }
            } else {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                // This correspond to Darwin implementation but there is a note in nRF52 implementation
                // that some iOS and HAT cannot handle GATT errors and get stuck.
                // nRF52 implementation disconnected central in this case.
                HAPLogError(&logObject, "%s: no delegate read handler", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
            }
        } else if (g_strcmp0(methodName, kBluezGattCharacteristicWriteValueMethod) == 0) {
            GVariantIter* valueIter;
            GVariantIter* optionIter;
            g_variant_get(parameters, "(aya{sv})", &valueIter, &optionIter);
            BeginRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
            CentralConnection* connection =
                    GetConnectionFromDeviceOption(characteristic->blePeripheralManager, optionIter);
            g_variant_iter_free(optionIter);
            if (!IsCentralStillConnected(
                        characteristic->blePeripheralManager,
                        characteristic->blePeripheralManager->gattObjectThread.dbusConnection,
                        connection)) {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                HAPLogInfo(&logObject, "%s: returning error to WriteValue for lack of central connection", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
                return;
            }
            uint8_t byte;
            uint8_t bytes[kHAPPlatformBLEPeripheralManager_MaxAttributeBytes];
            size_t numBytes = 0;
            while (g_variant_iter_loop(valueIter, "y", &byte)) {
                if (numBytes < sizeof bytes) {
                    bytes[numBytes] = byte;
                }
                numBytes++;
            }
            if (numBytes > sizeof bytes) {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                HAPLogError(&logObject, "%s: WriteValue with too many (%zu) bytes.", __func__, numBytes);
                g_dbus_method_invocation_return_dbus_error(
                        invocation, kBluezFailedError, kBluezGattNoResourcesErrorMessage);
            } else if (characteristic->blePeripheralManager->delegate.handleWriteRequest) {
                HAPError err = characteristic->blePeripheralManager->delegate.handleWriteRequest(
                        characteristic->blePeripheralManager->hapBLEPeripheralManager,
                        connection->handle,
                        characteristic->valueAttributeHandle,
                        bytes,
                        numBytes,
                        characteristic->blePeripheralManager->delegate.context);
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                if (err) {
                    HAPAssert(err == kHAPError_InvalidState || err == kHAPError_InvalidData);
                    // This correspond to Darwin implementation but there is a note in nRF52 implementation
                    // that some iOS and HAT cannot handle GATT errors and get stuck.
                    // nRF52 implementation disconnects central in this case.
                    // 0x04 => GATT_INVALID_PDU.
                    g_dbus_method_invocation_return_dbus_error(
                            invocation, kBluezFailedError, kBluezGattInvalidPduErrorMessage);
                } else {
                    g_dbus_method_invocation_return_value(invocation, NULL);
                }
            } else {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                HAPLogError(&logObject, "%s: no delegate write handler", __func__);
                g_dbus_method_invocation_return_dbus_error(invocation, kBluezNotPermittedError, NULL);
            }
        } else if (g_strcmp0(methodName, kBluezGattCharacteristicStartNotifyMethod) == 0) {
            BeginRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
            CentralConnection* connection = characteristic->blePeripheralManager->centralConnections;
            if (!IsCentralStillConnected(
                        characteristic->blePeripheralManager,
                        characteristic->blePeripheralManager->gattObjectThread.dbusConnection,
                        connection)) {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                HAPLogInfo(&logObject, "%s: StartNotify when no connection is available", __func__);
                // 0x81 => GATT_INTERNAL_ERROR
                g_dbus_method_invocation_return_dbus_error(
                        invocation, kBluezFailedError, kBluezGattInternalErrorMessage);
                return;
            }
            uint8_t bytes[2];
            // Enable events
            HAPWriteLittleUInt16(&bytes, 0x0002U);

            HAPError err = characteristic->blePeripheralManager->delegate.handleWriteRequest(
                    characteristic->blePeripheralManager->hapBLEPeripheralManager,
                    connection->handle,
                    characteristic->cccDescriptorAttributeHandle,
                    bytes,
                    sizeof bytes,
                    characteristic->blePeripheralManager->delegate.context);
            EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);

            if (err) {
                // This correspond to Darwin implementation but there is a note in nRF52 implementation
                // that some iOS and HAT cannot handle GATT errors and get stuck.
                // nRF52 implementation disconnects central in this case.
                // 0x04 => GATT_INVALID_PDU.
                g_dbus_method_invocation_return_dbus_error(
                        invocation, kBluezFailedError, kBluezGattInvalidPduErrorMessage);
            } else {
                g_dbus_method_invocation_return_value(invocation, NULL);
            }
        } else if (g_strcmp0(methodName, kBluezGattCharacteristicStopNotifyMethod) == 0) {
            BeginRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
            CentralConnection* connection = characteristic->blePeripheralManager->centralConnections;
            if (!IsCentralStillConnected(
                        characteristic->blePeripheralManager,
                        characteristic->blePeripheralManager->gattObjectThread.dbusConnection,
                        connection)) {
                EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);
                HAPLogInfo(&logObject, "%s: StopNotify when no connection is available", __func__);
                // 0x81 => GATT_INTERNAL_ERROR
                g_dbus_method_invocation_return_dbus_error(
                        invocation, kBluezFailedError, kBluezGattInternalErrorMessage);
                return;
            }
            uint8_t bytes[2];
            // Disable events
            HAPWriteLittleUInt16(&bytes, 0x0000U);

            HAPError err = characteristic->blePeripheralManager->delegate.handleWriteRequest(
                    characteristic->blePeripheralManager->hapBLEPeripheralManager,
                    connection->handle,
                    characteristic->cccDescriptorAttributeHandle,
                    bytes,
                    sizeof bytes,
                    characteristic->blePeripheralManager->delegate.context);
            EndRunloopSync(&characteristic->blePeripheralManager->gattObjectThread);

            if (err) {
                // This correspond to Darwin implementation but there is a note in nRF52 implementation
                // that some iOS and HAT cannot handle GATT errors and get stuck.
                // nRF52 implementation disconnects central in this case.
                // 0x04 => GATT_INVALID_PDU.
                g_dbus_method_invocation_return_dbus_error(
                        invocation, kBluezFailedError, kBluezGattInvalidPduErrorMessage);
            } else {
                g_dbus_method_invocation_return_value(invocation, NULL);
            }
        } else {
            HAPLogError(&logObject, "%s: Unknown method call: %s", __func__, methodName);
        }
    } else {
        HAPLogError(&logObject, "%s: Unknown interface method call: %s %s", __func__, interfaceName, methodName);
    }
}

/**
 * Get a property value of a DBUS GATT Characteristic object
 *
 * @param characteristic  DBUS GATT characteristic object
 * @param propertyName    property name
 *
 * @return GVariant object pointer corresponding to the property value
 */
static GVariant* GATTCharacteristicGetProperty(GATTCharacteristic* characteristic, const gchar* propertyName) {
    GVariant* ret = NULL;
    if (g_strcmp0(propertyName, kBluezGattCharacteristicUUIDProperty) == 0) {
        char bytes[kUUIDStringBufferSize];
        ret = g_variant_new_string(ConvertUUIDToString(characteristic->uuid, bytes, sizeof bytes));
    } else if (g_strcmp0(propertyName, kBluezGattCharacteristicServiceProperty) == 0) {
        char path[kMaxObjectPathLength];
        HAPError err = BuildGATTServiceObjectPath(characteristic->service, path, sizeof path);
        HAPAssert(!err);
        ret = g_variant_new_object_path(path);
    } else if (g_strcmp0(propertyName, kBluezGattCharacteristicDescriptorsProperty) == 0) {
        GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("ao"));
        GATTDescriptor* descriptor = characteristic->descriptors;
        while (descriptor) {
            char path[kMaxObjectPathLength];
            HAPError err = BuildGATTDescriptorObjectPath(descriptor, path, sizeof path);
            HAPAssert(!err);
            g_variant_builder_add(builder, "o", path);
            descriptor = descriptor->next;
        }
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else if (g_strcmp0(propertyName, kBluezGattCharacteristicFlagsProperty) == 0) {
        GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
        if (characteristic->properties.read) {
            g_variant_builder_add(builder, "s", kBluezGattCharacteristicFlagsRead);
        }
        if (characteristic->properties.writeWithoutResponse) {
            g_variant_builder_add(builder, "s", kBluezGattCharacteristicFlagsWriteWithoutResponse);
        }
        if (characteristic->properties.write) {
            g_variant_builder_add(builder, "s", kBluezGattCharacteristicFlagsWrite);
        }
        if (characteristic->properties.notify) {
            g_variant_builder_add(builder, "s", kBluezGattCharacteristicFlagsNotify);
        }
        if (characteristic->properties.indicate) {
            g_variant_builder_add(builder, "s", kBluezGattCharacteristicFlagsIndicate);
        }
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else {
        g_assert(FALSE);
    }
    return ret;
}

/**
 * Get all properties of DBUS GATT Characteristic object
 *
 * @param characteristic   characteristic
 * @param builder          variant builder to build the resulting property name and property value pairs as dictionary
 *                         entries.
 */
static void GATTCharacteristicGetAllProperties(GATTCharacteristic* characteristic, GVariantBuilder* builder) {
    const char* allProps[] = { kBluezGattCharacteristicUUIDProperty,
                               kBluezGattCharacteristicServiceProperty,
                               kBluezGattCharacteristicDescriptorsProperty,
                               kBluezGattCharacteristicFlagsProperty };
    for (size_t i = 0; i < HAPArrayCount(allProps); i++) {
        g_variant_builder_open(builder, G_VARIANT_TYPE("{sv}"));
        g_variant_builder_add(builder, "s", allProps[i]);
        g_variant_builder_add(builder, "v", GATTCharacteristicGetProperty(characteristic, allProps[i]));
        g_variant_builder_close(builder);
    }
}

/**
 * Handle GetProperty call to DBUS GATT Characteristic object
 */
static GVariant* GATTCharacteristicHandleGetProperty(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender HAP_UNUSED,
        const gchar* objectPath,
        const gchar* interfaceName,
        const gchar* propertyName,
        GError** error HAP_UNUSED,
        gpointer userData) {
    g_assert(g_strcmp0(interfaceName, kBluezGattCharacteristicInterface) == 0);
    g_assert(userData != NULL);
    GATTCharacteristic* characteristic = (GATTCharacteristic*) userData;

    g_printf("%s: %s %s %s\n", __func__, objectPath, interfaceName, propertyName);
    return GATTCharacteristicGetProperty(characteristic, propertyName);
}

/**
 * Create a DBUS GATT Characteristic object
 */
static guint CreateGATTCharacteristicObject(GDBusConnection* conn, GATTCharacteristic* characteristic) {
    HAPPrecondition(conn);
    HAPPrecondition(characteristic);

    const char introspectionXml[] =
            "<node>"
            "  <interface name='" kBluezGattCharacteristicInterface
            "'>"
            "    <method name='" kBluezGattCharacteristicReadValueMethod
            "'>"
            "      <arg name='options' type='a{sv}' direction='in'/>"
            "      <arg name='value' type='ay' direction='out'/>"
            "    </method>"
            "    <method name='" kBluezGattCharacteristicWriteValueMethod
            "'>"
            "      <arg name='value' type='ay' direction='in'/>"
            "      <arg name='options' type='a{sv}' direction='in'/>"
            "    </method>"
            "    <method name='" kBluezGattCharacteristicStartNotifyMethod
            "'/>"
            "    <method name='" kBluezGattCharacteristicStopNotifyMethod
            "'/>"
            "    <property type='s' name='" kBluezGattCharacteristicUUIDProperty
            "' access='read'/>"
            "    <property type='o' name='" kBluezGattCharacteristicServiceProperty
            "' access='read'/>"
            "    <property type='ay' name='" kBluezGattCharacteristicFlagsProperty
            "' access='read'/>"
            "    <property type='ao' name='" kBluezGattCharacteristicDescriptorsProperty
            "' access='read'/>"
            "  </interface>"
            "</node>";
    GError* error = NULL;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(introspectionXml, &error);
    if (error) {
        HAPLogError(&logObject, "%s:%s", __func__, error->message);
        g_error_free(error);
        return 0;
    }

    GDBusInterfaceVTable vtable;
    vtable.method_call = GATTCharacteristicHandleMethodCall;
    vtable.get_property = GATTCharacteristicHandleGetProperty;
    vtable.set_property = NULL;

    char path[kMaxObjectPathLength];
    HAPError err = BuildGATTCharacteristicObjectPath(characteristic, path, sizeof path);
    HAPAssert(!err);
    guint registrationId = g_dbus_connection_register_object(
            conn, path, nodeInfo->interfaces[0], &vtable, characteristic, NULL, &error);
    g_dbus_node_info_unref(nodeInfo);
    if (error) {
        HAPLogError(&logObject, "%s:%s", __func__, error->message);
        g_error_free(error);
        return 0;
    }
    return registrationId;
}

/**
 * Get property value of DBUS GATT Service object
 *
 * @param service       GATT service object
 * @param propertyName  property name
 *
 * @return GVariant object pointer corresponding to the property value
 */
static GVariant* GATTServiceGetProperty(GATTService* service, const gchar* propertyName) {
    GVariant* ret = NULL;
    if (g_strcmp0(propertyName, kBluezGattServiceUUIDProperty) == 0) {
        char buffer[kUUIDStringBufferSize];
        ret = g_variant_new_string(ConvertUUIDToString(service->uuid, buffer, sizeof buffer));
    } else if (g_strcmp0(propertyName, kBluezGattServicePrimaryProperty) == 0) {
        ret = g_variant_new("b", (gboolean) service->isPrimary);
    } else if (g_strcmp0(propertyName, kBluezGattServiceCharacteristicsProperty) == 0) {
        GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("ao"));
        GATTCharacteristic* characteristic = service->characteristics;
        while (characteristic) {
            char path[kMaxObjectPathLength];
            HAPError err = BuildGATTCharacteristicObjectPath(characteristic, path, sizeof path);
            HAPAssert(!err);
            g_variant_builder_add(builder, "o", path);
            characteristic = characteristic->next;
        }
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else {
        g_assert(FALSE);
    }

    return ret;
}

/**
 * Get all properties of DBUS GATT Service object
 *
 * @param service       GATT service object
 * @param[out] builder  GVariant builder to build the property and value pairs as
 *                      dictionary entries.
 */
static void GATTServiceGetAllProperties(GATTService* service, GVariantBuilder* builder) {
    const char* allProps[] = { kBluezGattServiceUUIDProperty,
                               kBluezGattServicePrimaryProperty,
                               kBluezGattServiceCharacteristicsProperty };
    for (size_t i = 0; i < HAPArrayCount(allProps); i++) {
        g_variant_builder_open(builder, G_VARIANT_TYPE("{sv}"));
        g_variant_builder_add(builder, "s", allProps[i]);
        g_variant_builder_add(builder, "v", GATTServiceGetProperty(service, allProps[i]));
        g_variant_builder_close(builder);
    }
}

/**
 * Handle DBUS GATT Service object GetProperty call
 */
static GVariant* GATTServiceHandleGetProperty(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender HAP_UNUSED,
        const gchar* objectPath,
        const gchar* interfaceName,
        const gchar* propertyName,
        GError** error HAP_UNUSED,
        gpointer userData) {
    g_assert(g_strcmp0(interfaceName, kBluezGattServiceInterface) == 0);
    g_assert(userData != NULL);
    GATTService* service = (GATTService*) userData;

    HAPLogDebug(&logObject, "%s: %s %s %s\n", __func__, objectPath, interfaceName, propertyName);
    return GATTServiceGetProperty(service, propertyName);
}

/**
 * Create DBUS GATT Service object
 */
static guint CreateGATTServiceObject(GDBusConnection* conn, GATTService* service) {
    HAPPrecondition(service);

    const char introspectionXml[] =
            "<node>"
            "  <interface name='" kBluezGattServiceInterface
            "'>"
            "    <property type='s' name='" kBluezGattServiceUUIDProperty
            "' access='read'/>"
            "    <property type='b' name='" kBluezGattServicePrimaryProperty
            "' access='read'/>"
            "    <property type='ao' name='" kBluezGattServiceCharacteristicsProperty
            "' access='read'/>"
            "  </interface>"
            "</node>";
    GError* error = NULL;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(introspectionXml, &error);
    if (error) {
        HAPLogError(&logObject, "%s:%s", __func__, error->message);
        g_error_free(error);
        return 0;
    }

    GDBusInterfaceVTable vtable;
    vtable.method_call = NULL;
    vtable.get_property = GATTServiceHandleGetProperty;
    vtable.set_property = NULL;

    char path[kMaxObjectPathLength];
    HAPError err = BuildGATTServiceObjectPath(service, path, sizeof path);
    HAPAssert(!err);
    guint registrationId =
            g_dbus_connection_register_object(conn, path, nodeInfo->interfaces[0], &vtable, service, NULL, &error);
    g_dbus_node_info_unref(nodeInfo);
    if (error) {
        HAPLogError(&logObject, "%s:%s", __func__, error->message);
        g_error_free(error);
        return 0;
    }
    return registrationId;
}

/**
 * Handle method call to DBUS GATT application object
 */
static void GATTApplicationHandleMethodCall(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender HAP_UNUSED,
        const gchar* objectPath HAP_UNUSED,
        const gchar* interfaceName,
        const gchar* methodName,
        GVariant* parameters HAP_UNUSED,
        GDBusMethodInvocation* invocation,
        gpointer userData) {
    g_assert(userData != NULL);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) userData;

    if (g_strcmp0(interfaceName, kDBusObjectManagerInterface) == 0) {
        if (g_strcmp0(methodName, kDBusObjectManagerGetManagedObjectsMethod) == 0) {
            GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("a{oa{sa{sv}}}"));
            GATTService* service = blePeripheralManager->services;
            while (service) {
                g_variant_builder_open(builder, G_VARIANT_TYPE("{oa{sa{sv}}}"));
                char path[kMaxObjectPathLength];
                HAPError err = BuildGATTServiceObjectPath(service, path, sizeof path);
                HAPAssert(!err);
                g_variant_builder_add(builder, "o", path);
                g_variant_builder_open(builder, G_VARIANT_TYPE("a{sa{sv}}"));
                g_variant_builder_open(builder, G_VARIANT_TYPE("{sa{sv}}"));
                g_variant_builder_add(builder, "s", kBluezGattServiceInterface);
                g_variant_builder_open(builder, G_VARIANT_TYPE("a{sv}"));
                GATTServiceGetAllProperties(service, builder);
                g_variant_builder_close(builder);
                g_variant_builder_close(builder);
                g_variant_builder_close(builder);
                g_variant_builder_close(builder);
                GATTCharacteristic* characteristic = service->characteristics;
                while (characteristic) {
                    g_variant_builder_open(builder, G_VARIANT_TYPE("{oa{sa{sv}}}"));
                    HAPError err = BuildGATTCharacteristicObjectPath(characteristic, path, sizeof path);
                    HAPAssert(!err);
                    g_variant_builder_add(builder, "o", path);
                    g_variant_builder_open(builder, G_VARIANT_TYPE("a{sa{sv}}"));
                    g_variant_builder_open(builder, G_VARIANT_TYPE("{sa{sv}}"));
                    g_variant_builder_add(builder, "s", kBluezGattCharacteristicInterface);
                    g_variant_builder_open(builder, G_VARIANT_TYPE("a{sv}"));
                    GATTCharacteristicGetAllProperties(characteristic, builder);
                    g_variant_builder_close(builder);
                    g_variant_builder_close(builder);
                    g_variant_builder_close(builder);
                    g_variant_builder_close(builder);
                    GATTDescriptor* descriptor = characteristic->descriptors;
                    while (descriptor) {
                        g_variant_builder_open(builder, G_VARIANT_TYPE("{oa{sa{sv}}}"));
                        HAPError err = BuildGATTDescriptorObjectPath(descriptor, path, sizeof path);
                        HAPAssert(!err);
                        g_variant_builder_add(builder, "o", path);
                        g_variant_builder_open(builder, G_VARIANT_TYPE("a{sa{sv}}"));
                        g_variant_builder_open(builder, G_VARIANT_TYPE("{sa{sv}}"));
                        g_variant_builder_add(builder, "s", kBluezGattDescriptorInterface);
                        g_variant_builder_open(builder, G_VARIANT_TYPE("a{sv}"));
                        GATTDescriptorGetAllProperties(descriptor, builder);
                        g_variant_builder_close(builder);
                        g_variant_builder_close(builder);
                        g_variant_builder_close(builder);
                        g_variant_builder_close(builder);
                        descriptor = descriptor->next;
                    }
                    characteristic = characteristic->next;
                }
                service = service->next;
            }
            GVariant* ret = g_variant_builder_end(builder);
            g_variant_builder_unref(builder);
            g_dbus_method_invocation_return_value(invocation, g_variant_new_tuple(&ret, 1));
        } else {
            HAPFatalError();
        }
    } else {
        HAPFatalError();
    }
}

/**
 * Create DBUS GATT application object
 */
static guint CreateGATTApplicationObject(GDBusConnection* conn, BLEPeripheralManager* blePeripheralManager) {
    HAPPrecondition(blePeripheralManager);

    const char introspectionXml[] =
            "<node>"
            "  <interface name='" kDBusObjectManagerInterface
            "'>"
            "    <method name='" kDBusObjectManagerGetManagedObjectsMethod
            "'>"
            "      <arg name='objs' type='a{oa{sa{sv}}}' direction='out'/>"
            "    </method>"
            "  </interface>"
            "</node>";
    GError* error = NULL;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(introspectionXml, &error);
    if (error) {
        HAPLogError(&logObject, "%s: %s\n", __func__, error->message);
        g_error_free(error);
        return 0;
    }

    GDBusInterfaceVTable vtable;
    vtable.method_call = GATTApplicationHandleMethodCall;
    vtable.get_property = NULL;
    vtable.set_property = NULL;

    char path[kMaxObjectPathLength];
    HAPError err = BuildGATTServiceObjectRootPath(blePeripheralManager, path, sizeof path);
    HAPAssert(!err);
    guint registrationId = g_dbus_connection_register_object(
            conn, path, nodeInfo->interfaces[0], &vtable, blePeripheralManager, NULL, &error);
    g_dbus_node_info_unref(nodeInfo);
    if (error) {
        HAPLogError(&logObject, "%s: %s\n", __func__, error->message);
        g_error_free(error);
        return 0;
    }
    return registrationId;
}

static void AdvertisementHandleMethodCall(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender HAP_UNUSED,
        const gchar* objectPath,
        const gchar* interfaceName,
        const gchar* methodName,
        GVariant* parameters HAP_UNUSED,
        GDBusMethodInvocation* invocation HAP_UNUSED,
        gpointer userData HAP_UNUSED) {
    if (g_strcmp0(interfaceName, kBluezLEAdvertisementInterface) == 0) {
        if (g_strcmp0(methodName, kBluezLEAdvertisementReleaseMethod) == 0) {
            HAPLogDebug(&logObject, "%s: advertising instance %s is released", __func__, objectPath);
        } else {
            g_assert(FALSE);
        }
    } else {
        g_assert(FALSE);
    }
}

/**
 * Handle GetProperty call to the DBUS advertisement object
 */
static GVariant* AdvertisementHandleGetProperty(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* sender HAP_UNUSED,
        const gchar* objectPath HAP_UNUSED,
        const gchar* interfaceName,
        const gchar* propertyName,
        GError** error HAP_UNUSED,
        gpointer userData) {
    GVariant* ret;

    ret = NULL;
    HAPAssert(g_strcmp0(interfaceName, kBluezLEAdvertisementInterface) == 0);
    HAPPrecondition(userData);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) userData;

    if (g_strcmp0(propertyName, kBluezLEAdvertisementTypeProperty) == 0) {
        ret = g_variant_new_string(
                blePeripheralManager->advertisement.isPeripheral ? kBluezLEAdvertisementTypePeripheral :
                                                                   kBluezLEAdvertisementTypeBroadcast);
    } else if (g_strcmp0(propertyName, kBluezLEAdvertisementManufacturerDataProperty) == 0) {
        GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("a{qv}"));
        AdvertisementMfgData* mfgData = blePeripheralManager->advertisement.mfgData;
        while (mfgData) {
            GVariantBuilder* arrayBuilder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
            for (size_t i = 0; i < mfgData->numBytes; i++) {
                g_variant_builder_add(arrayBuilder, "y", ((guchar*) (mfgData + 1))[i]);
            }
            GVariant* array = g_variant_builder_end(arrayBuilder);
            g_variant_builder_unref(arrayBuilder);
            g_variant_builder_add(builder, "{qv}", mfgData->companyId, array);
            mfgData = mfgData->next;
        }
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else if (
            g_strcmp0(propertyName, kBluezLEAdvertisementServiceUUIDsProperty) == 0 ||
            g_strcmp0(propertyName, kBluezLEAdvertisementSolicitUUIDsProperty) == 0) {
        GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else if (g_strcmp0(propertyName, kBluezLEAdvertisementServiceDataProperty) == 0) {
        GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
        ret = g_variant_builder_end(builder);
        g_variant_builder_unref(builder);
    } else if (g_strcmp0(propertyName, kBluezLEAdvertisementIncludeTxPowerProperty) == 0) {
        ret = g_variant_new_boolean(FALSE);
    } else if (g_strcmp0(propertyName, kBluezLEAdvertisementLocalNameProperty) == 0) {
        ret = g_variant_new_string(blePeripheralManager->advertisement.localName);
    } else if (g_strcmp0(propertyName, kBluezLEAdvertisementDurationProperty) == 0) {
        ret = g_variant_new_uint16(2);
    } else if (g_strcmp0(propertyName, kBluezLEAdvertisementDiscoverableProperty) == 0) {
        ret = g_variant_new_boolean(blePeripheralManager->advertisement.isDiscoverable);
    } else if (g_strcmp0(propertyName, kBluezLEAdvertisementIntervalProperty) == 0) {
        ret = g_variant_new_uint16(blePeripheralManager->advertisement.interval);
    } else {
        g_assert(FALSE);
    }

    return ret;
}

/**
 * Create DBUS advertisement object
 *
 * @param conn                  DBUS connection
 * @param blePeripheralManager  ble peripheral manager
 * @return registration ID of the created advertisement object, or zero if failed.
 */
static guint CreateAdvertisementObject(GDBusConnection* conn, BLEPeripheralManager* blePeripheralManager) {
    char advertisementIntrospectionXmlBuffer[1024];
    HAPError err = HAPStringWithFormat(
            advertisementIntrospectionXmlBuffer,
            sizeof advertisementIntrospectionXmlBuffer,
            "<node>"
            "  <interface name='" kBluezLEAdvertisementInterface
            "'>"
            "    <method name='" kBluezLEAdvertisementReleaseMethod
            "'/>"
            "    <property type='s' name='" kBluezLEAdvertisementTypeProperty
            "' access='read'/>"
            "    <property type='q' name='" kBluezLEAdvertisementDurationProperty
            "' access='read'/>"
            "    %s%s%s"
            "    <property type='q' name='" kBluezLEAdvertisementIntervalProperty
            "' access='read'/>"
            "  </interface>"
            "</node>",
            blePeripheralManager->advertisement.mfgData ?
                    "<property type='a{qv}' name='" kBluezLEAdvertisementManufacturerDataProperty "' access='read'/>" :
                    "",
            blePeripheralManager->advertisement.localName ?
                    "<property type='s' name='" kBluezLEAdvertisementLocalNameProperty "' access='read'/>" :
                    "",
            blePeripheralManager->advertisement.isDiscoverableOverriden ?
                    "<property type='b' name='" kBluezLEAdvertisementDiscoverableProperty "' access='read'/>" :
                    "");
    HAPAssert(!err);
    GError* error = NULL;
    GDBusNodeInfo* nodeInfo = g_dbus_node_info_new_for_xml(advertisementIntrospectionXmlBuffer, &error);
    if (error) {
        HAPLogError(&logObject, "%s: g_dbus_node_info_new_for_xml failed: %s", __func__, error->message);
        g_error_free(error);
        return 0;
    }

    static GDBusInterfaceVTable advertisementVTable;
    advertisementVTable.method_call = AdvertisementHandleMethodCall;
    advertisementVTable.get_property = AdvertisementHandleGetProperty;
    advertisementVTable.set_property = NULL;

    char path[kMaxObjectPathLength];
    err = BuildAdvertisementObjectPath(blePeripheralManager, path, sizeof path);
    HAPAssert(!err);
    // Note that the following glib call sets up introspect and properties method and hence no need
    // to add introspect and properties interfaces and methods in the above XML and vtable.
    guint registrationId = g_dbus_connection_register_object(
            conn, path, nodeInfo->interfaces[0], &advertisementVTable, blePeripheralManager, NULL, &error);
    g_dbus_node_info_unref(nodeInfo);
    if (error) {
        HAPLogError(&logObject, "%s: g_dbus_connection_register_object failed: %s", __func__, error->message);
        g_error_free(error);
        return 0;
    }
    return registrationId;
}

/**
 * Handle Bluez interface changed signals
 */
void HandleBluezSignals(
        GDBusConnection* connection HAP_UNUSED,
        const gchar* senderName,
        const gchar* objectPath,
        const gchar* interfaceName,
        const gchar* signalName,
        GVariant* parameters,
        gpointer userData) {
    HAPPrecondition(userData);
    DBusObjectThread* thread = (DBusObjectThread*) userData;

    if (g_strcmp0(interfaceName, kDBusPropertiesInterface) == 0 &&
        g_strcmp0(signalName, kBluezPropertisChangedSignal) == 0) {

        HAPLogDebug(&logObject, "%s: %s %s %s %s", __func__, senderName, objectPath, interfaceName, signalName);

        char* interfaceName;
        GVariantIter* changedPropertyIter;
        GVariantIter* invalidatedPropertyIter;
        g_variant_get(parameters, "(sa{sv}as)", &interfaceName, &changedPropertyIter, &invalidatedPropertyIter);
        if (g_strcmp0(interfaceName, kBluezDeviceInterface) == 0) {
            char* propertyName;
            GVariant* value;
            bool isConnectedStateAffected = false;
            bool isConnected = false;
            while (g_variant_iter_loop(changedPropertyIter, "{sv}", &propertyName, &value)) {
                if (g_strcmp0(propertyName, kBluezDeviceConnectedProperty) == 0) {
                    isConnectedStateAffected = true;
                    isConnected = g_variant_get_boolean(value);
                }
            }
            while (g_variant_iter_loop(invalidatedPropertyIter, "s", &propertyName)) {
                if (g_strcmp0(propertyName, kBluezDeviceConnectedProperty) == 0) {
                    isConnectedStateAffected = true;
                    isConnected = false;
                }
            }
            if (isConnectedStateAffected) {
                HAPLogDebug(&logObject, "%s: %s connected state changed %d", __func__, objectPath, isConnected ? 1 : 0);
                BeginRunloopSync(thread);
                CentralConnection* connection =
                        FindBLEPeripheralManagerCentralConnection(thread->blePeripheralManager, objectPath);
                if (!connection) {
                    EndRunloopSync(thread);
                    g_free(interfaceName);
                    g_variant_iter_free(changedPropertyIter);
                    g_variant_iter_free(invalidatedPropertyIter);
                    HAPLogError(&logObject, "%s: Delegate connection state could not be updated.", __func__);
                    return;
                }
                if (isConnected && !connection->isReportedAsConnected) {
                    // Connection was not reported to the peripheral delegate yet. Report it.
                    if (thread->blePeripheralManager->delegate.handleConnectedCentral) {
                        thread->blePeripheralManager->delegate.handleConnectedCentral(
                                thread->blePeripheralManager->hapBLEPeripheralManager,
                                connection->handle,
                                thread->blePeripheralManager->delegate.context);
                    }
                    connection->isReportedAsConnected = true;
                } else if (!isConnected && connection->isReportedAsConnected) {
                    if (thread->blePeripheralManager->delegate.handleDisconnectedCentral) {
                        thread->blePeripheralManager->delegate.handleDisconnectedCentral(
                                thread->blePeripheralManager->hapBLEPeripheralManager,
                                connection->handle,
                                thread->blePeripheralManager->delegate.context);
                    }
                    connection->isReportedAsConnected = false;
                }
                if (!isConnected) {
                    DropBLEPeripheralManagerCentralConnection(thread->blePeripheralManager, objectPath);
                }
                EndRunloopSync(thread);
            }
        }
        g_free(interfaceName);
        g_variant_iter_free(changedPropertyIter);
        g_variant_iter_free(invalidatedPropertyIter);
    }
}

/**
 * Subscribe to interface changed signals of Bluez stack
 */
void SubscribeToBluezInterfaceChangedEvents(gpointer data) {
    HAPPrecondition(data);

    DBusObjectThread* thread = (DBusObjectThread*) data;
    HAPPrecondition(thread->blePeripheralManager);
    HAPPrecondition(thread->blePeripheralManager->signalReceiverThread.dbusConnection);
    thread->blePeripheralManager->bluezSubscriptionId = g_dbus_connection_signal_subscribe(
            thread->blePeripheralManager->signalReceiverThread.dbusConnection,
            kBluezBus,
            NULL,
            NULL,
            NULL,
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            HandleBluezSignals,
            data,
            NULL);
}

void HAPPlatformBLEPeripheralManagerCreate(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager,
        const HAPPlatformBLEPeripheralManagerOptions* options) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(options);

    HAPPlatformBLECheckBlueZVersion();

    BLEPeripheralManager* obj = malloc(sizeof(BLEPeripheralManager));
    if (!obj) {
        HAPFatalError();
    }
    HAPRawBufferZero(obj, sizeof *obj);
    obj->hciResourceIndex = -1;
    HAPError err = AllocateHCIResource(options->hciPath, &obj->hciResourceIndex);
    if (err) {
        free(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }
    int ret = sem_init(&obj->syncSemaphore, 0, 0);
    if (ret) {
        FreeBLEPeripheralManager(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }
    obj->isSyncSemaphoreValid = true;
    obj->hapBLEPeripheralManager = blePeripheralManager;

    // Setup signal receiver thread
    err = LaunchDBusObjectThread(&obj->signalReceiverThread, "signal rx thread", obj);
    if (err) {
        FreeBLEPeripheralManager(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }

    // Setup GATT object thread
    err = LaunchDBusObjectThread(&obj->gattObjectThread, "gatt obj thread", obj);
    if (err) {
        FreeBLEPeripheralManager(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }

    // Setup Advertisement object thread
    err = LaunchDBusObjectThread(&obj->advertisementThread, "advert thread", obj);
    if (err) {
        FreeBLEPeripheralManager(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }

    // Setup DBUS connection to use directly from RunLoop thread.
    GError* error = NULL;
    obj->dbusConnection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error) {
        HAPLogError(&logObject, "%s: g_bus_get_sync failed %s", __func__, error->message);
        g_error_free(error);
        obj->dbusConnection = NULL;
        FreeBLEPeripheralManager(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }

    obj->serviceRootEnumId = hciResources[obj->hciResourceIndex].nextServiceRootEumId++;
    obj->nextFakeAttributeHandle = 1;
    obj->advertisement.isPeripheral = options->isConnectable;
    blePeripheralManager->internal = obj;

    // Connect to Bluez stack interface changed signal
    err = InvokeSyncFunction(
            obj->signalReceiverThread.mainContext,
            &obj->syncSemaphore,
            SubscribeToBluezInterfaceChangedEvents,
            &obj->signalReceiverThread);
    if (err) {
        FreeBLEPeripheralManager(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }

    // Power the adapter
    error = NULL;
    GVariant* result = g_dbus_connection_call_sync(
            obj->dbusConnection,
            kBluezBus,
            hciResources[obj->hciResourceIndex].hciPath,
            kDBusPropertiesInterface,
            "Set",
            g_variant_new("(ssv)", kBluezAdapterInterface, kBluezAdapterPoweredProperty, g_variant_new_boolean(true)),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);
    if (error) {
        HAPLogError(
                &logObject,
                "%s: could not power on the adapter %s: %s",
                __func__,
                hciResources[obj->hciResourceIndex].hciPath,
                error->message);
        g_error_free(error);
        FreeBLEPeripheralManager(obj);
        blePeripheralManager->internal = NULL;
        HAPFatalError();
    }
    if (result) {
        g_variant_unref(result);
    }
}

void HAPPlatformBLEPeripheralManagerSetDelegate(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerDelegate* _Nullable delegate) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);
    if (delegate) {
        HAPRawBufferCopyBytes(&blePeripheralManager->delegate, delegate, sizeof blePeripheralManager->delegate);
    } else {
        HAPRawBufferZero(&blePeripheralManager->delegate, sizeof blePeripheralManager->delegate);
    }
}

void HAPPlatformBLEPeripheralManagerSetDeviceAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager HAP_UNUSED,
        const HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress HAP_UNUSED) {
    HAPLogInfo(&logObject, "Cannot set device address in the current platform.");
}

HAPError HAPPlatformBLEPeripheralManagerGetAdvertisementAddress(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        HAPPlatformBLEPeripheralManagerDeviceAddress* deviceAddress) {
    HAPPrecondition(blePeripheralManager_);
    HAPPrecondition(deviceAddress);

    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    if (!blePeripheralManager) {
        HAPLogError(&logObject, "%s: called with invalid state peripheral manager", __func__);
        return kHAPError_InvalidState;
    }

    GError* error = NULL;
    char* hciPath = hciResources[blePeripheralManager->hciResourceIndex].hciPath;
    GVariant* result = g_dbus_connection_call_sync(
            blePeripheralManager->dbusConnection,
            kBluezBus,
            hciPath,
            kDBusPropertiesInterface,
            kDBusPropertiesGetMethod,
            g_variant_new("(ss)", kBluezAdapterInterface, kBluezAdapterAddressProperty),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

    if (error) {
        HAPLogDebug(&logObject, "%s: couldn't get Address property of %s: %s", __func__, hciPath, error->message);
        g_error_free(error);
        return kHAPError_Unknown;
    }

    HAPAssert(g_variant_n_children(result) == 1);
    GVariant* element = g_variant_get_child_value(result, 0);
    GVariant* elementValue = g_variant_get_variant(element);
    char* address;
    g_variant_get(elementValue, "s", &address);
    HAPLogDebug(&logObject, "%s: retrieved address = %s", __func__, address);
    HAPAssert(HAPStringGetNumBytes(address) == 17);
    for (size_t i = 0; i < 6; i++) {
        char hexByte[3];
        HAPRawBufferCopyBytes(hexByte, &address[15 - (i * 3)], 2);
        hexByte[2] = 0;
        HAPError err = HAPUInt8FromHexString(hexByte, &deviceAddress->bytes[i]);
        HAPAssert(!err);
    }
    g_free(address);
    g_variant_unref(elementValue);
    g_variant_unref(element);
    g_variant_unref(result);

    result = g_dbus_connection_call_sync(
            blePeripheralManager->dbusConnection,
            kBluezBus,
            hciPath,
            kDBusPropertiesInterface,
            kDBusPropertiesGetMethod,
            g_variant_new("(ss)", kBluezAdapterInterface, kBluezAdapterAddressTypeProperty),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

    if (error) {
        HAPLogDebug(&logObject, "%s: couldn't get AddressType property of %s: %s", __func__, hciPath, error->message);
        g_error_free(error);
        return kHAPError_Unknown;
    }

    HAPAssert(g_variant_n_children(result) == 1);
    element = g_variant_get_child_value(result, 0);
    elementValue = g_variant_get_variant(element);
    char* addressType;
    g_variant_get(elementValue, "s", &addressType);
    HAPLogDebug(&logObject, "%s: retrieved addressType = %s", __func__, addressType);
    bool isPublic = HAPStringAreEqual(addressType, "public");
    g_free(addressType);
    g_variant_unref(elementValue);
    g_variant_unref(element);
    g_variant_unref(result);

    return (isPublic ? kHAPError_None : kHAPError_Unknown);
}

void HAPPlatformBLEPeripheralManagerSetDeviceName(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager HAP_UNUSED,
        const char* deviceName HAP_UNUSED) {
    HAPLogInfo(&logObject, "Cannot set device name in the current platform.");
}

bool HAPPlatformBLEPeripheralManagerAllowsServiceRefresh(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager HAP_UNUSED) {
    return true;
}

/**
 * Unregister and remove all service objects from DBus object thread
 */
static void RemoveAllServiceObjects(gpointer data) {
    HAPPrecondition(data);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) data;

    // Deregister and remove all DBUS objects
    GATTService* service = blePeripheralManager->services;
    while (service) {
        GATTCharacteristic* characteristic = service->characteristics;
        while (characteristic) {
            GATTDescriptor* descriptor = characteristic->descriptors;
            while (descriptor) {
                g_dbus_connection_unregister_object(
                        blePeripheralManager->gattObjectThread.dbusConnection, descriptor->registrationId);
                GATTDescriptor* tmpDescriptor = descriptor;
                descriptor = descriptor->next;
                free(tmpDescriptor);
            }
            g_dbus_connection_unregister_object(
                    blePeripheralManager->gattObjectThread.dbusConnection, characteristic->registrationId);
            GATTCharacteristic* tmpCharacteristic = characteristic;
            characteristic = characteristic->next;
            free(tmpCharacteristic);
        }
        g_dbus_connection_unregister_object(
                blePeripheralManager->gattObjectThread.dbusConnection, service->registrationId);
        GATTService* tmpService = service;
        service = service->next;
        free(tmpService);
    }
    blePeripheralManager->services = NULL;
}

void HAPPlatformBLEPeripheralManagerRemoveAllServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager_) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->dbusConnection);

    if (blePeripheralManager->isGATTAppRegistered) {
        // Deregister the GATT application
        char path[kMaxObjectPathLength];
        HAPError err = BuildGATTServiceObjectRootPath(blePeripheralManager, path, sizeof path);
        HAPAssert(!err);
        GError* error = NULL;
        GVariant* result = g_dbus_connection_call_sync(
                blePeripheralManager->dbusConnection,
                kBluezBus,
                hciResources[blePeripheralManager->hciResourceIndex].hciPath,
                kBluezGattManagerInterface,
                kBluezGattManagerUnregisterApplicationMethod,
                g_variant_new("(o)", path),
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                -1,
                NULL,
                &error);
        g_assert_no_error(error);
        g_variant_unref(result);
        blePeripheralManager->isGATTAppRegistered = false;
    }

    HAPError err = InvokeSyncFunction(
            blePeripheralManager->gattObjectThread.mainContext,
            &blePeripheralManager->syncSemaphore,
            RemoveAllServiceObjects,
            blePeripheralManager);
    HAPAssert(!err);
}

/** AddService function argument */
typedef struct {
    BLEPeripheralManager* blePeripheralManager;
    const HAPPlatformBLEPeripheralManagerUUID* type;
    bool isPrimary;
    HAPError result;
} AddServiceArguments;

/** AddService function to be executed from service object thread */
static void AddService(gpointer data) {
    HAPPrecondition(data);
    AddServiceArguments* args = (AddServiceArguments*) data;

    if (args->blePeripheralManager->gattApplicationRegistrationId == 0) {
        // GATT application object is not registered yet
        args->blePeripheralManager->gattApplicationRegistrationId = CreateGATTApplicationObject(
                args->blePeripheralManager->gattObjectThread.dbusConnection, args->blePeripheralManager);
        if (args->blePeripheralManager->gattApplicationRegistrationId == 0) {
            args->result = kHAPError_Unknown;
            return;
        }
    }

    GATTService* service = (GATTService*) malloc(sizeof(GATTService) + sizeof(HAPPlatformBLEPeripheralManagerUUID));
    if (!service) {
        args->result = kHAPError_OutOfResources;
        return;
    }
    HAPRawBufferZero(service, sizeof *service);
    HAPRawBufferCopyBytes(service + 1, args->type, sizeof(*(args->type)));
    service->next = args->blePeripheralManager->services;
    service->blePeripheralManager = args->blePeripheralManager;
    service->uuid = (HAPPlatformBLEPeripheralManagerUUID*) (service + 1);
    service->enumId = args->blePeripheralManager->nextServiceEnumId;
    service->isPrimary = args->isPrimary;

    // Create the service object
    service->registrationId =
            CreateGATTServiceObject(args->blePeripheralManager->gattObjectThread.dbusConnection, service);
    if (service->registrationId == 0) {
        args->result = kHAPError_Unknown;
        free(service);
        return;
    }
    args->blePeripheralManager->nextServiceEnumId++;
    args->blePeripheralManager->services = service;
    args->result = kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddService(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        bool isPrimary) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);

    AddServiceArguments args = {
        .blePeripheralManager = blePeripheralManager,
        .type = type,
        .isPrimary = isPrimary,
        .result = kHAPError_None,
    };
    HAPError err = InvokeSyncFunction(
            blePeripheralManager->gattObjectThread.mainContext,
            &blePeripheralManager->syncSemaphore,
            AddService,
            &args);
    if (err) {
        return err;
    }
    return args.result;
}

/** AddCharacteristic function argument data structure */
typedef struct {
    BLEPeripheralManager* blePeripheralManager;
    const HAPPlatformBLEPeripheralManagerUUID* type;
    HAPPlatformBLEPeripheralManagerCharacteristicProperties properties;
    const void* _Nullable constBytes;
    size_t constNumBytes;
    HAPPlatformBLEPeripheralManagerAttributeHandle* valueHandle;
    HAPPlatformBLEPeripheralManagerAttributeHandle* _Nullable cccDescriptorHandle;
    HAPError result;
} AddCharacteristicArguments;

/** AddCharacteristic function to be executed from service object thread */
static void AddCharacteristic(gpointer data) {
    HAPPrecondition(data);
    AddCharacteristicArguments* args = (AddCharacteristicArguments*) data;

    HAPPrecondition(args);
    HAPPrecondition(args->blePeripheralManager);
    HAPPrecondition(args->blePeripheralManager->gattApplicationRegistrationId != 0);

    GATTCharacteristic* characteristic = (GATTCharacteristic*) malloc(
            sizeof(GATTCharacteristic) + sizeof(HAPPlatformBLEPeripheralManagerUUID) +
            (args->constBytes ? args->constNumBytes : 0));
    if (!characteristic) {
        args->result = kHAPError_OutOfResources;
        return;
    }
    HAPRawBufferZero(characteristic, sizeof *characteristic);
    HAPRawBufferCopyBytes(characteristic + 1, args->type, sizeof *(args->type));
    characteristic->uuid = (HAPPlatformBLEPeripheralManagerUUID*) (characteristic + 1);
    if (args->constBytes) {
        HAPRawBufferCopyBytes((void*) (characteristic->uuid + 1), args->constBytes, args->constNumBytes);
        characteristic->constBytes = characteristic->uuid + 1;
    } else {
        characteristic->constBytes = NULL;
    }

    // Characteristic is associated with the most recently added service
    HAPPrecondition(args->blePeripheralManager->services);
    characteristic->service = args->blePeripheralManager->services;
    characteristic->next = characteristic->service->characteristics;

    characteristic->blePeripheralManager = args->blePeripheralManager;
    characteristic->constNumBytes = args->constNumBytes;
    characteristic->enumId = characteristic->service->nextCharacteristicEnumId;
    characteristic->valueAttributeHandle = args->blePeripheralManager->nextFakeAttributeHandle;
    characteristic->cccDescriptorAttributeHandle = characteristic->valueAttributeHandle + 1;
    characteristic->properties = args->properties;

    // Create the characteristic object
    characteristic->registrationId =
            CreateGATTCharacteristicObject(args->blePeripheralManager->gattObjectThread.dbusConnection, characteristic);
    if (characteristic->registrationId == 0) {
        args->result = kHAPError_Unknown;
        free(characteristic);
        return;
    }
    characteristic->service->nextCharacteristicEnumId++;
    args->blePeripheralManager->nextFakeAttributeHandle += 2;
    characteristic->service->characteristics = characteristic;
    if (args->valueHandle) {
        *(args->valueHandle) = characteristic->valueAttributeHandle;
    }
    if (args->cccDescriptorHandle) {
        *(args->cccDescriptorHandle) = characteristic->cccDescriptorAttributeHandle;
    }
    args->result = kHAPError_None;
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
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);

    AddCharacteristicArguments args = {
        blePeripheralManager, type,        properties,          constBytes,
        constNumBytes,        valueHandle, cccDescriptorHandle, kHAPError_None,
    };
    HAPError err = InvokeSyncFunction(
            blePeripheralManager->gattObjectThread.mainContext,
            &blePeripheralManager->syncSemaphore,
            AddCharacteristic,
            &args);
    if (err) {
        return err;
    }
    return args.result;
}

/** AddDescriptor function argument data structure */
typedef struct {
    BLEPeripheralManager* blePeripheralManager;
    const HAPPlatformBLEPeripheralManagerUUID* type;
    HAPPlatformBLEPeripheralManagerDescriptorProperties properties;
    const void* _Nullable constBytes;
    size_t constNumBytes;
    HAPPlatformBLEPeripheralManagerAttributeHandle* descriptorHandle;
    HAPError result;
} AddDescriptorArguments;

/** AddDescriptor function to be executed from service object thread */
static void AddDescriptor(gpointer data) {
    HAPPrecondition(data);
    AddDescriptorArguments* args = (AddDescriptorArguments*) data;

    HAPPrecondition(args);
    HAPPrecondition(args->blePeripheralManager);
    HAPPrecondition(args->blePeripheralManager->gattApplicationRegistrationId != 0);

    GATTDescriptor* descriptor = (GATTDescriptor*) malloc(
            sizeof(GATTDescriptor) + sizeof(HAPPlatformBLEPeripheralManagerUUID) +
            (args->constBytes ? args->constNumBytes : 0));
    if (!descriptor) {
        args->result = kHAPError_OutOfResources;
        return;
    }
    HAPRawBufferZero(descriptor, sizeof *descriptor);
    HAPRawBufferCopyBytes(descriptor + 1, args->type, sizeof *(args->type));
    descriptor->uuid = (HAPPlatformBLEPeripheralManagerUUID*) (descriptor + 1);
    if (args->constBytes) {
        HAPRawBufferCopyBytes((void*) (descriptor->uuid + 1), args->constBytes, args->constNumBytes);
        descriptor->constBytes = descriptor->uuid + 1;
    } else {
        descriptor->constBytes = NULL;
    }

    // Descriptor is associated with the most recently added characteristic
    HAPPrecondition(args->blePeripheralManager->services);
    HAPPrecondition(args->blePeripheralManager->services->characteristics);
    descriptor->characteristic = args->blePeripheralManager->services->characteristics;
    descriptor->next = descriptor->characteristic->descriptors;

    descriptor->blePeripheralManager = args->blePeripheralManager;
    descriptor->constNumBytes = args->constNumBytes;
    descriptor->enumId = descriptor->characteristic->nextDescriptorEnumId;
    descriptor->descriptorHandle = args->blePeripheralManager->nextFakeAttributeHandle;
    descriptor->properties = args->properties;

    // Create the descriptor object
    descriptor->registrationId =
            CreateGATTDescriptorObject(args->blePeripheralManager->gattObjectThread.dbusConnection, descriptor);
    if (descriptor->registrationId == 0) {
        args->result = kHAPError_Unknown;
        free(descriptor);
        return;
    }
    descriptor->characteristic->nextDescriptorEnumId++;
    args->blePeripheralManager->nextFakeAttributeHandle++;
    descriptor->characteristic->descriptors = descriptor;
    if (args->descriptorHandle) {
        *(args->descriptorHandle) = descriptor->descriptorHandle;
    }
    args->result = kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerAddDescriptor(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        const HAPPlatformBLEPeripheralManagerUUID* type,
        HAPPlatformBLEPeripheralManagerDescriptorProperties properties,
        const void* _Nullable constBytes,
        size_t constNumBytes,
        HAPPlatformBLEPeripheralManagerAttributeHandle* descriptorHandle) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);

    AddDescriptorArguments args = {
        blePeripheralManager, type, properties, constBytes, constNumBytes, descriptorHandle, kHAPError_None,
    };
    HAPError err = InvokeSyncFunction(
            blePeripheralManager->gattObjectThread.mainContext,
            &blePeripheralManager->syncSemaphore,
            AddDescriptor,
            &args);
    if (err) {
        return err;
    }
    return args.result;
}

void HAPPlatformBLEPeripheralManagerPublishServices(HAPPlatformBLEPeripheralManagerRef blePeripheralManager_) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->dbusConnection);

    if (blePeripheralManager->gattApplicationRegistrationId == 0) {
        HAPLogDebug(&logObject, "%s: no service registered.", __func__);
        return;
    }

    GVariantBuilder* optionsBuilder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    GVariant* options = g_variant_builder_end(optionsBuilder);
    g_variant_builder_unref(optionsBuilder);
    GVariant** tupleElements = g_new(GVariant*, 2);
    char path[kMaxObjectPathLength];
    HAPError err = BuildGATTServiceObjectRootPath(blePeripheralManager, path, sizeof path);
    HAPAssert(!err);
    tupleElements[0] = g_variant_new("o", path);
    tupleElements[1] = options;
    GVariant* args = g_variant_new_tuple(tupleElements, 2);
    g_free(tupleElements);

    GError* error = NULL;
    GVariant* result = g_dbus_connection_call_sync(
            blePeripheralManager->dbusConnection,
            kBluezBus,
            hciResources[blePeripheralManager->hciResourceIndex].hciPath,
            kBluezGattManagerInterface,
            kBluezGattManagerRegisterApplicationMethod,
            args,
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);
    g_assert_no_error(error);
    g_variant_unref(result);
    blePeripheralManager->isGATTAppRegistered = true;
}

/**
 * Start advertisement
 *
 * @param conn        DBUS connection
 * @param hciPath     DBUS hci object path
 * @param advobjpath  DBUS advertisement object path
 *
 * @return kHAPError_None when successful. kHAPError_Unknown when failed.
 */
static HAPError StartAdvertisement(GDBusConnection* conn, const char* hciPath, const char* advobjpath) {
    HAPLogDebug(&logObject, "%s", __func__);
    GVariantBuilder* optionsBuilder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    GVariant* options = g_variant_builder_end(optionsBuilder);
    g_variant_builder_unref(optionsBuilder);
    GVariant** tupleElements = g_new(GVariant*, 2);
    tupleElements[0] = g_variant_new("o", advobjpath);
    tupleElements[1] = options;
    GVariant* args = g_variant_new_tuple(tupleElements, 2);
    g_free(tupleElements);

    GError* error = NULL;
    GVariant* result = g_dbus_connection_call_sync(
            conn,
            kBluezBus,
            hciPath,
            kBluezLEAdvertisingManagerInterface,
            kBluezLEAdvertisingManagerRegisterAdvertisementMethod,
            args,
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);
    if (result) {
        g_variant_unref(result);
    }
    if (error) {
        HAPLogError(&logObject, "%s: %s", __func__, error->message);
        g_error_free(error);
        return kHAPError_Unknown;
    }
    HAPLogDebug(&logObject, "%s -- done", __func__);
    return kHAPError_None;
}

/**
 * Stop advertisement
 *
 * @param conn        DBUS connection
 * @param hciPath     DBUS hci object path
 * @param advobjpath  DBUS advertisement object path
 */
static void StopAdvertisement(GDBusConnection* conn, const char* hciPath, const char* advobjpath) {
    HAPLogDebug(&logObject, "%s", __func__);
    GError* error = NULL;
    GVariant* result = g_dbus_connection_call_sync(
            conn,
            kBluezBus,
            hciPath,
            kBluezLEAdvertisingManagerInterface,
            kBluezLEAdvertisingManagerUnregisterAdvertisementMethod,
            g_variant_new("(o)", advobjpath),
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);
    if (error) {
        HAPLogError(&logObject, "%s: %s", __func__, error->message);
        g_error_free(error);
    }
    if (result) {
        g_variant_unref(result);
    }
    HAPLogDebug(&logObject, "%s -- done", __func__);
}

/**
 * Parse raw advertising bytes and scan response bytes
 *
 * @param blePeripheralManager   ble peripheral manager which will hold parsed advertising data
 * @param advertisingBytes       data to be embedded in ADV_IND
 * @param numAdvertisingBytes    number of bytes of advertisingBytes
 * @param scanResponseBytes      data to be embedded in SCAN_RESP
 * @param numScanResponseBytes   number of bytes of scanResponseBytes
 *
 * @return kHAPError_None when successful.<br>
 *         kHAPError_OutOfResources when ran out of memory.<br>
 *         kHAPError_InvalidData when data cannot be parsed.<br>
 */
static HAPError ParseRawAdvertisementData(
        BLEPeripheralManager* blePeripheralManager,
        const uint8_t* advertisingBytes,
        size_t numAdvertisingBytes,
        const uint8_t* _Nullable scanResponseBytes,
        size_t numScanResponseBytes) {
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(advertisingBytes);

    const uint8_t* sourceBytes[] = { advertisingBytes, scanResponseBytes };
    size_t sourceNumBytes[] = { numAdvertisingBytes, scanResponseBytes ? numScanResponseBytes : 0 };

    for (size_t i = 0; i < HAPArrayCount(sourceBytes); i++) {
        const uint8_t* remainingBytes = sourceBytes[i];
        size_t numRemainingBytes = sourceNumBytes[i];

        while (numRemainingBytes > 0) {
            uint8_t length = *remainingBytes++;
            numRemainingBytes--;
            if (length > numRemainingBytes) {
                HAPLogError(&logObject, "%s: bad length", __func__);
                return kHAPError_InvalidData;
            }
            if (length > 0) {
                uint8_t adType = *remainingBytes;
                switch (adType) {
                    case 0xFF: {
                        // manufacturer data
                        if (length < 3) {
                            HAPLogError(&logObject, "%s: bad mfg data length", __func__);
                            FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
                            return kHAPError_InvalidData;
                        }
                        AdvertisementMfgData* mfgData = malloc(sizeof(AdvertisementMfgData) + length - 3);
                        if (!mfgData) {
                            HAPLogError(&logObject, "%s: mfg data malloc(%u) failed", __func__, length - 3);
                            FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
                            return kHAPError_OutOfResources;
                        }
                        HAPRawBufferZero(mfgData, sizeof *mfgData);
                        mfgData->numBytes = length - 3;
                        mfgData->companyId = HAPReadLittleUInt16(&remainingBytes[1]);
                        HAPRawBufferCopyBytes(mfgData + 1, remainingBytes + 3, length - 3);

                        // Append the manufacturing data to the last so that the advertisement data is also sent in the
                        // same order.
                        AdvertisementMfgData** ptr = &blePeripheralManager->advertisement.mfgData;
                        while (*ptr) {
                            ptr = &(*ptr)->next;
                        }
                        *ptr = mfgData;
                        break;
                    }
                    case 0x09: {
                        // complete local name
                        if (blePeripheralManager->advertisement.localName) {
                            HAPLogError(&logObject, "%s: duplicate local name", __func__);
                            FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
                            return kHAPError_InvalidData;
                        }
                        blePeripheralManager->advertisement.localName = malloc(length);
                        if (!blePeripheralManager->advertisement.localName) {
                            HAPLogError(&logObject, "%s: local name malloc(%u) failed", __func__, length);
                            FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
                            return kHAPError_OutOfResources;
                        }
                        HAPRawBufferCopyBytes(
                                blePeripheralManager->advertisement.localName, remainingBytes + 1, length - 1);
                        blePeripheralManager->advertisement.localName[length - 1] = 0;
                        break;
                    }
                    case 0x01: {
                        // Flags
                        HAPAssert(length == 2);
                        blePeripheralManager->advertisement.isDiscoverable = ((remainingBytes[1] & 0x2) != 0);
                        blePeripheralManager->advertisement.isDiscoverableOverriden = false;
                        break;
                    }
                    case 0x08: {
                        // Shortened local name cannot be forced and hence ignore them.
                        break;
                    }
                    default: {
                        HAPLogError(&logObject, "%s: Unexpected ad type 0x%X", __func__, adType);
                        return kHAPError_InvalidData;
                    }
                }
                remainingBytes += length;
                numRemainingBytes -= length;
            }
        }
    }
    return kHAPError_None;
}

/** Argument to CreateAdvertisementObjectAndSaveState function */
typedef struct {
    BLEPeripheralManager* blePeripheralManager;
    HAPError result;
} CreateAdvertisementObjectAndSaveStateArgs;

/** Create an advertisement object and store the result into BLE peripheral manager */
static void CreateAdvertisementObjectAndSaveState(gpointer data) {
    HAPPrecondition(data);
    CreateAdvertisementObjectAndSaveStateArgs* args = data;

    args->blePeripheralManager->advertisement.registrationId = CreateAdvertisementObject(
            args->blePeripheralManager->advertisementThread.dbusConnection, args->blePeripheralManager);
    if (args->blePeripheralManager->advertisement.registrationId == 0) {
        args->result = kHAPError_Unknown;
    } else {
        args->result = kHAPError_None;
    }
}

/** Remove the DBUS advertisement object */
static void RemoveAdvertisementObject(gpointer data) {
    HAPPrecondition(data);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) data;

    g_dbus_connection_unregister_object(
            blePeripheralManager->advertisementThread.dbusConnection,
            blePeripheralManager->advertisement.registrationId);
    blePeripheralManager->advertisement.registrationId = 0;
}

void HAPPlatformBLEPeripheralManagerStartAdvertising(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        HAPBLEAdvertisingInterval advertisingInterval,
        const void* advertisingBytes,
        size_t numAdvertisingBytes,
        const void* _Nullable scanResponseBytes,
        size_t numScanResponseBytes) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);

    HAPLogDebug(&logObject, "%s", __func__);

    if (!blePeripheralManager->advertisement.isPeripheral &&
        advertisingInterval < HAPBLEAdvertisingIntervalCreateFromMilliseconds(100)) {
        HAPLogError(&logObject, "%s: advertising interval too short for non connectable", __func__);
        return;
    }

    char path[kMaxObjectPathLength];
    HAPError err = BuildAdvertisementObjectPath(blePeripheralManager, path, sizeof path);
    HAPAssert(!err);
    if (blePeripheralManager->advertisement.isAdvertising) {
        StopAdvertisement(
                blePeripheralManager->dbusConnection,
                hciResources[blePeripheralManager->hciResourceIndex].hciPath,
                path);
        blePeripheralManager->advertisement.isAdvertising = false;
        if (blePeripheralManager->advertisement.registrationId) {
            HAPError err = InvokeSyncFunction(
                    blePeripheralManager->advertisementThread.mainContext,
                    &blePeripheralManager->syncSemaphore,
                    RemoveAdvertisementObject,
                    blePeripheralManager);
            if (err) {
                HAPLogError(&logObject, "%s: Invoking RemoveAdvertisementObject failed", __func__);
                HAPFatalError();
            }
        }
        FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
    }

    // Because advertising raw data is not supported in this platform,
    // decode the advertisement raw data and build the data into blePeripheralManager.
    err = ParseRawAdvertisementData(
            blePeripheralManager, advertisingBytes, numAdvertisingBytes, scanResponseBytes, numScanResponseBytes);
    if (err) {
        // Parsing failed. Cannot advertise. The above function must have already logged an error.
        return;
    }

    CreateAdvertisementObjectAndSaveStateArgs args = {
        blePeripheralManager,
        0,
    };

    // Set advertising interval
    blePeripheralManager->advertisement.interval = advertisingInterval;

    err = InvokeSyncFunction(
            blePeripheralManager->advertisementThread.mainContext,
            &blePeripheralManager->syncSemaphore,
            CreateAdvertisementObjectAndSaveState,
            &args);
    if (err) {
        return;
    }
    if (args.result) {
        return;
    }

    err = StartAdvertisement(
            blePeripheralManager->dbusConnection, hciResources[blePeripheralManager->hciResourceIndex].hciPath, path);
    if (err) {
        // Starting advertisement failed.
        // Remove DBUS advertisement object
        HAPError err = InvokeSyncFunction(
                blePeripheralManager->advertisementThread.mainContext,
                &blePeripheralManager->syncSemaphore,
                RemoveAdvertisementObject,
                blePeripheralManager);
        if (err) {
            HAPLogError(&logObject, "%s: Invoking RemoveAdvertisementObject failed", __func__);
            HAPFatalError();
        }
        FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
    } else {
        blePeripheralManager->advertisement.isAdvertising = true;
    }
    HAPLogDebug(&logObject, "%s -- done", __func__);
}

void HAPPlatformBLEPeripheralManagerStopAdvertising(HAPPlatformBLEPeripheralManagerRef blePeripheralManager_) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);
    HAPLogDebug(&logObject, "%s", __func__);

    char path[kMaxObjectPathLength];
    HAPError err = BuildAdvertisementObjectPath(blePeripheralManager, path, sizeof path);
    HAPAssert(!err);

    if (blePeripheralManager->advertisement.isAdvertising) {
        StopAdvertisement(
                blePeripheralManager->dbusConnection,
                hciResources[blePeripheralManager->hciResourceIndex].hciPath,
                path);
        blePeripheralManager->advertisement.isAdvertising = false;
        if (blePeripheralManager->advertisement.registrationId) {
            HAPError err = InvokeSyncFunction(
                    blePeripheralManager->advertisementThread.mainContext,
                    &blePeripheralManager->syncSemaphore,
                    RemoveAdvertisementObject,
                    blePeripheralManager);
            if (err) {
                HAPLogError(&logObject, "%s: Invoking RemoveAdvertisementObject failed", __func__);
                HAPFatalError();
            }
        }
        FreeBLEPeripheralManagerAdvertisement(blePeripheralManager);
    }
    HAPLogDebug(&logObject, "%s -- done", __func__);
}

void HAPPlatformBLEPeripheralManagerCancelCentralConnection(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);

    CentralConnection* connection =
            FindBLEPeripheralManagerCentralConnectionFromHandle(blePeripheralManager, connectionHandle);
    if (!connection) {
        // By race condition, connection might have been removed.
        HAPLogInfo(&logObject, "%s: Central connection is not present.", __func__);
        return;
    }
    if (!connection->isReportedAsConnected) {
        // By race condition, connection state might have changed.
        HAPLogInfo(&logObject, "%s: Central is not connected.", __func__);
        return;
    }

    GVariant* result = g_dbus_connection_call_sync(
            blePeripheralManager->dbusConnection,
            kBluezBus,
            connection->objPath,
            kBluezDeviceInterface,
            kBluezDeviceDisconnectMethod,
            NULL,
            NULL,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            NULL);
    if (result) {
        g_variant_unref(result);
    }

    // The connection will be removed when the connection is actually disconnected
    // and hence do not remove it here.
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformBLEPeripheralManagerSendHandleValueIndication(
        HAPPlatformBLEPeripheralManagerRef blePeripheralManager_,
        HAPPlatformBLEPeripheralManagerConnectionHandle connectionHandle HAP_UNUSED,
        HAPPlatformBLEPeripheralManagerAttributeHandle valueHandle,
        const void* _Nullable bytes,
        size_t numBytes) {
    HAPPrecondition(blePeripheralManager_);
    BLEPeripheralManager* blePeripheralManager = (BLEPeripheralManager*) blePeripheralManager_->internal;
    HAPPrecondition(blePeripheralManager);
    HAPPrecondition(blePeripheralManager->dbusConnection);

    GATTCharacteristic* characteristic = FindCharacteristicOfValueHandle(blePeripheralManager, valueHandle);
    if (!characteristic) {
        // Note that this API doesn't allow returning an error code indicating bad parameter.
        // In a PAL implementation that can send a BLE handle value blindly,
        // it will send out bad value handle successfully which probably would be ignored by the central.
        // Since sending of bad value handle shouldn't bring about anything, we can pretend that it is done.
        HAPLogError(&logObject, "%s: bad value handle %u", __func__, valueHandle);
        return kHAPError_None;
    }
    GVariantBuilder* valueBuilder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
    for (size_t i = 0; i < numBytes; i++) {
        g_variant_builder_add(valueBuilder, "y", ((guchar*) bytes)[i]);
    }
    GVariant* value = g_variant_builder_end(valueBuilder);
    g_variant_builder_unref(valueBuilder);
    GVariantBuilder* propertiesBuilder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(propertiesBuilder, "{sv}", kBluezGattCharacteristicValueProperty, value);
    GVariant* changedProperties = g_variant_builder_end(propertiesBuilder);
    g_variant_builder_unref(propertiesBuilder);
    propertiesBuilder = g_variant_builder_new(G_VARIANT_TYPE("as"));
    GVariant* invalidatedProperties = g_variant_builder_end(propertiesBuilder);
    g_variant_builder_unref(propertiesBuilder);
    GVariant** tupleElements = g_new(GVariant*, 3);
    tupleElements[0] = g_variant_new("s", kBluezGattCharacteristicInterface);
    tupleElements[1] = changedProperties;
    tupleElements[2] = invalidatedProperties;
    GVariant* args = g_variant_new_tuple(tupleElements, 3);
    g_free(tupleElements);

    char path[kMaxObjectPathLength];
    HAPError err = BuildGATTCharacteristicObjectPath(characteristic, path, sizeof path);
    HAPAssert(!err);

    GError* error = NULL;
    g_dbus_connection_emit_signal(
            blePeripheralManager->dbusConnection,
            kBluezBus,
            path,
            kDBusPropertiesInterface,
            kBluezPropertisChangedSignal,
            args,
            &error);
    if (error) {
        HAPLogError(&logObject, "%s: %s", __func__, error->message);
        g_error_free(error);
        return kHAPError_InvalidState;
    }
    return kHAPError_None;
}

#else // HAP_FEATURE_ENABLED(HAP_FEATURE_PLATFORM_BLE)
#include "../Mock/HAPPlatformBLEPeripheralManager.c"
#endif
