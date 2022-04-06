Accessory Development
=====================

An accessory logic developer can typically start from one of the sample applications provided in the ADK and modify it
based on development needs.

## Sample Application

The sample applications provided in the ADK are structured as follows:

### DB.h and DB.c

DB.c contains a declaration of the HomeKit services and characteristics that the accessory should support, called the
accessory attribute database. It is a *C* data structure that also contains pointers to functions that will be called by
the HAP Library when the accessory needs to perform an action. The functions themselves are implemented in the separate
files App.h and App.c. For an overview of the Apple-defined HomeKit profiles, please review the current version of the
*HomeKit Accessory Protocol Specification* document available on the MFi portal.

### App.h and App.c

App.c contains the implementation of the actual accessory logic, as functions that are called by the HAP Library
whenever the accessory needs to perform an action. For example, the `HandleThermostatTargetTemperatureWrite` function
must be implemented for a thermostat. It switches on a heating element when the user increases the thermostat’s target
temperature. The original DB.h / DB.c / App.c / App.h samples provided by Apple are portable across platforms. Console
output is used to simulate the behavior of these accessories. Chipset vendors may provide additional samples that
demonstrate vendor-specific aspects, e.g., how to access GPIOs or PWMs for controlling an LED. Accessory manufacturers
modify these files according to the needs of their accessories.

### AppBase.c

This `C` file contains the main function with code for initializing the accessory logic state and the PAL, possibly
termination code, and code for controlling the HomeKit accessory server that is implemented in the HAP Library. All the
applications share the same `AppBase.c` file using a symbolic link to `Applications/AppBase.c`.

Please refer to [ADK Directory Structure](./adk_directory_structure.md) for more details on how the Application files
are arranged.

## Accessory Logic

This section describes the steps for implementing the accessory logic for a new product. Use the *LightBulb* or
*Thermostat* sample applications as a starting point for light bulbs or thermostats. The same samples can be used as
starting points for other relatively simple accessories (e.g., leak sensor, switch, fan, etc.). Use the *Lock* sample
application as a starting point for door locks. Use the *IPCamera* and *VideoDoorbell* sample applications as starting
point for cameras and video doorbells. Use the *Bridge* sample application for HomeKit bridges.

### Define the Attribute Database

Define the accessory attribute database with the IIDs of the involved services and characteristics (e.g., in DB.c) by
modifying the selected sample code. Definition is done in a declarative way, by setting up C data structures for
accessories, services and characteristics. The details of these elements are described in the document *HomeKit
Accessory Protocol Specification*, in the chapters Apple-defined Characteristics, Apple-defined Services and
Apple-defined Profiles.

In `App.c`, an accessory object is defined in this manner:

```c
static HAPAccessory accessory = {
    .aid = 1,
    .category = kHAPAccessoryCategory_Lighting,
    .name = "Acme LightBulb",
    .manufacturer = "Acme",
    .model = "LightBulb1,1",
    .serialNumber = "099DB48E9E28",
    .firmwareVersion = "1",
    .hardwareVersion = "1",
    .services = (const HAPService *const[]) {
        &accessoryInformationService,
        &hapProtocolInformationService,
        &pairingService,
        &lightbulbService,
        NULL
    },
    .cameraStreamConfigurations = NULL,
    .callbacks = {
        .identify = IdentifyAccessory
    }
};
```

- The accessory instance id aid must be set to 1 for non-bridged accessories and for HomeKit bridges themselves. For
bridged accessories, it must be a number in the range 2 to 264 - 1.
- The accessory object contains an array of service objects. In this case an *Accessory Information service*, a
*Protocol Information service*, a *Pairing service*, and a *LightBulb service*. Every accessory object must provide an
*Accessory Information service* and the *Protocol Information service*. Every *BLE* accessory must provide the
*Pairing service*, which is ignored on *IP* accessories.
- The `const` keyword is used in order to enable the toolchain to put the accessory attribute database into flash memory
on systems where RAM is scarce.
- After a firmware update, the field firmwareVersion must be incremented. Everything else required according to the
*HomeKit Accessory Protocol specification* is done by the HAP Library, e.g., incrementing the config number (CN).
Note that firmware downgrades are not allowed.

To decouple the aspects of an accessory that are product-specific from those that are typically the same, or very
similar, for all accessories of a particular HomeKit profile, the accessory attribute database is placed in the file
`DB.c`. There, declarations of instance IDs for the supported services and their characteristics are needed:

```c
#define kIID_AccessoryInformation                      ((uint64_t) 0x0001)
#define kIID_AccessoryInformationIdentify              ((uint64_t) 0x0002)
#define kIID_AccessoryInformationManufacturer          ((uint64_t) 0x0003)
#define kIID_AccessoryInformationModel                 ((uint64_t) 0x0004)
#define kIID_AccessoryInformationName                  ((uint64_t) 0x0005)
#define kIID_AccessoryInformationSerialNumber          ((uint64_t) 0x0006)
#define kIID_AccessoryInformationFirmwareRevision      ((uint64_t) 0x0007)
#define kIID_AccessoryInformationHardwareRevision      ((uint64_t) 0x0008)
#define kIID_AccessoryInformationADKVersion            ((uint64_t) 0x0009)
#define kIID_AccessoryInformationProductData           ((uint64_t) 0x000A)

#define kIID_HAPProtocolInformation                    ((uint64_t) 0x0010)
#define kIID_HAPProtocolInformationServiceSignature    ((uint64_t) 0x0011)
#define kIID_HAPProtocolInformationVersion             ((uint64_t) 0x0012)

#define kIID_Pairing                                   ((uint64_t) 0x0020)
#define kIID_PairingPairSetup                          ((uint64_t) 0x0022)
#define kIID_PairingPairVerify                         ((uint64_t) 0x0023)
#define kIID_PairingPairingFeatures                    ((uint64_t) 0x0024)
#define kIID_PairingPairingPairings                    ((uint64_t) 0x0025)

#define kIID_LightBulb                                 ((uint64_t) 0x0030)
#define kIID_LightBulbServiceSignature                 ((uint64_t) 0x0031)
#define kIID_LightBulbName                             ((uint64_t) 0x0032)
#define kIID_LightBulbOn                               ((uint64_t) 0x0033)

HAP_STATIC_ASSERT(kAttributeCount == 10 + 3 + 5 + 4, AttributeCount_mismatch);
```

- An instance ID iid must be in the range of `1 to 2^64 - 1` for *IP* accessories, `1 to 2^16 - 1` for *BLE*
accessories. Once an accessory is paired with its controller, an IID must remain the same throughout the lifetime of the
accessory pairing. This means in particular across firmware updates, and also when someone migrates an accessory from a
non-ADK based HomeKit SDK to one that is based on the ADK. Also, an IID that is not used anymore, after a firmware
update, must not be reused for anything else. IIDs are chosen from a single ID pool for all characteristics and services
of an accessory, except for a HomeKit bridge where every bridged accessory has its own pool.
- The IIDs above are grouped by services. The sample accessory supports the Accessory Information, HAP-BLE
*Protocol Information*, *Pairing* and *LightBulb services*. The first two are mandatory HomeKit services. The
*Pairing service* is a HomeKit service required only for *BLE* accessories. It is recommended to define it even for
*IP* accessories, so that adding *BLE* support later on would be simplified.
- The samples are structured such that their implementation of all three HomeKit system services
(*Accessory Information*, HAP-BLE *Protocol Information*, and *Pairing*) can be reused as is, with only their IIDs
possibly having to be modified. This is possible because accessory-specific information that is exposed particularly in
the *Accessory Information* service, such as manufacturer and serialNumber, are already provided in the `HAPAccessory`
object in the separate file `App.c`.
- The *Accessory Information* service must contain the *ADKVersion* characteristic, see the `DB.c` files of the sample
Applications.
- The assertion check after the IID definitions helps detect inconsistencies, especially after changing the set of
services or after adding characteristics. kAttributeCount is the total number of IIDs and must be correct. Both
characteristics and services are attributes with IIDs, and therefore must be counted.

A LightBulb service is defined in this manner:

```c
const HAPService lightbulbService = {
    .iid = kIID_LightBulb,
    .serviceType = &kHAPServiceType_LightBulb,
    .debugDescription = kHAPServiceDebugDescription_LightBulb,
    .name = "Light Bulb",
    .properties = {
        .primaryService = true,
        .hidden = false,
        .ble = {
            .supportsConfiguration = false,
        },
    },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic *const[]) {
        &lightbulbServiceSignatureCharacteristic,
        &lightbulbNameCharacteristic,
        &lightbulbOnCharacteristic,
        NULL,
    },
};
```

The service object contains an array of characteristic objects. In this case a *Service Signature* characteristic, a
*Name* characteristic, and an *On* characteristic. The latter controls the on/off state of the light bulb.

An *On* characteristic is defined in this manner:

```c
const HAPBoolCharacteristic lightbulbOnCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_LightBulbOn,
    .characteristicType = &kHAPCharacteristicType_On,
    .debugDescription = kHAPCharacteristicDebugDescription_On,
    .manufacturerDescription = NULL,
    .properties = {
        .readable = true,
        .writable = true,
        .supportsEventNotification = true,
        .hidden = false,
        .requiresTimedWrite = false,
        .supportsAuthorizationData = false,
        .ip = {
            .controlPoint = false
        },
        .ble = {
            .supportsBroadcastNotification = true,
            .supportsDisconnectedNotification = true,
            .readableWithoutSecurity = false,
            .writableWithoutSecurity = false
        }
    },
    .callbacks = {
        .handleRead = HandleLightBulbOnRead,
        .handleWrite = HandleLightBulbOnWrite
    }
};
```

- This is a Boolean characteristic, as indicated by its type HAPBoolCharacteristic, so its value at runtime will be
either true or false. HAP.h supports a number of other types for characteristics, e.g., integer and floating point
numbers, strings, etc.
    - Make sure that the type matches the format, e.g., a HAPBoolCharacteristic must have
    `kHAPCharacteristicFormat_Bool`, a `HAPUInt8Characteristic` must have `kHAPCharacteristicFormat_UInt8`, etc.
- The characteristic object contains two callbacks: `handleRead` and `handleWrite`. They are bound to the functions
`HandleLightBulbOnRead` and `HandleLightBulbOnWrite`.
- The `lightbulbServiceSignatureCharacteristic` is ignored on *IP* accessories, but is required on *BLE* accessories for
services that are linked, primary, hidden or support configuration. See HAP.h (search for “service properties”) or the
*HomeKit Accessory Protocol Specification* for more information.

### Implement Callbacks

Implement the callbacks that provide the actual behavior of the accessory, e.g., to change the set point of a
thermostat (in `App.c`). Callbacks are blocking, meaning it is the accessory logic’s responsibility to immediately
process them without waiting. This is possible because HAP is essentially a protocol for the remote reading and writing
of variables (characteristics) over a network, which should not involve lengthy processing on the accessory.
In the light bulb sample application, these two functions are typical examples of callback implementations:

```c
HAPError HandleLightBulbOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.lightbulbOn;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}
```

and

```c
HAPError HandleLightBulbOnWrite(
        HAPAccessoryServerRef* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : “false”);
    if (accessoryConfiguration.state.lightbulbOn != value) {
        accessoryConfiguration.state.lightbulbOn = value;
        SaveAccessoryState();
        if (value) {
            TurnOnLightBulb();
        } else {
            TurnOffLightBulb();
        }
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}
```

- `HAPLogInfo` is used to produce log output that can be easily inspected during development. In a real accessory, the
implementations of TurnOnLightBulb() and TurnOffLightBulb() would contain the code to query the platform for the state
of the GPIO, PWM or other mechanism for controlling the light-emitting element of the light bulb. `HAPLogInfo` relies on
the portable `HAPStringWithFormat` function for string-formatting to minimize dependencies on the *C* standard library.
Especially on small embedded systems, the various C standard libraries do not support all possible features such as
`%ll` or `%z` format specifiers. The `HAPStringWithFormat` function only offers a subset of the full *printf* style format
specifiers:
    - flags:  0, +, ' '
    - width:  number
    - length: l, ll, z
    - types:  %, d, i, u, x, X, p, s, c, g

``` Note::
    Precisions as well as dynamic widths are not supported.
```

- The variable accessoryConfiguration contains the accessory logic state, in this case the state of the light bulb
(on/off).
- `HAPAccessoryServerRaiseEvent` tells the HAP server that this characteristic has changed, which may require sending of
notifications. This function must always be called after the value of a characteristic was changed by the accessory
logic.
- `HAPAccessoryServerRef` is an example of an opaque pointer. Accessory logic must access its internals only via the
access functions provided. For custom implementations (e.g., `HAPPlatformKeyValueStoreRef` in the PAL), the ADK only
accesses it via the access functions needed, never directly.

### Start the Application

After setting up the accessory attribute database descriptors with their callbacks, initialize the platform and then
call the functions `HAPAccessoryServerCreate` and `HAPAccessoryServerStart` with the descriptors as arguments. Then call
the `HAPPlatformRunLoopRun`, which handles all protocol processing and invokes the callbacks when needed. This whole
initialization part can be platform-specific, and therefore is placed in a separate file for the sample program
(`AppBase.c`). Also, it may be specific to the transport being used (here: *IP* only, without *BLE*) and specific to the
accessory category (here: light bulb).

The central object is a HomeKit accessory server, declared as follows:

```c
static HAPAccessoryServerRef accessoryServer;
```

Accessory initialization sequence:

- `HAPPlatformSetupDrivers` - Initialize platform drivers
- `HAPPlatformSetupInitKeyValueStore` - Initialize Key Value store
- `HAPPlatformSetupInitBLE` or `HAPPlatformSetupInitThread` - Initialize *BLE* or *THREAD* peripheral managers.
- `HAPPlatformTCPStreamManagerCreate` - Create a TCP stream manager for an *IP* accessory. For an *IP* accessory, a TCP
stream manager object must be provided.
- `HAPPlatformMFiTokenAuthCreate` - Initialize Software Token provider if Software Token Authentication is used.
- `HAPPlatformRunLoopCreate` - Create run loop, which manages and dispatches I/O events and drives the scheduling of
timer events.
- Initialize `HAPIPAccessoryServerStorage` for an *IP* accessory with the memory required for incoming *IP* sessions
based on the maximum number of simultaneous *IP* sessions supported by the accessory. Similarly initialize
`HAPBLEAccessoryServerStorage` for a *BLE* accessory and `HAPThreadAccessoryServerStorage` for a *THREAD* accessory.
- `HAPAccessoryServerCreate` - Initialize accessory server
- `HAPAccessoryServerStart` - Start the accessory server
- `HAPPlatformRunLoopRun` - Start the platform run loop. This is a blocking call.

Please note:
 - `HAPAccessoryServerCreate` has a `HAPAccessoryServerOptions` parameter, which controls whether HAP over *IP*, *BLE*
 or *THREAD* is used, and for HAP over *IP*, whether or not Wi-Fi (and thus WAC2) is used.
 - Storage needed for the Application is also provided in the `HAPAccessoryServerOptions` parameter.
- `HAPAccessoryServerCreate` has a `HAPPlatform` parameter, which must embody the PAL services to be used by the HAP
library. In the example, the platform parameters have first been combined in a global variable to make platform
dependencies easy to find:

```c
static struct {
    HAPPlatformKeyValueStore keyValueStore;
    HAPPlatformAccessorySetup accessorySetup;
#if (HAVE_DISPLAY == 1)
    HAPPlatformAccessorySetupDisplay setupDisplay;
#endif
#if (HAVE_NFC == 1)
    HAPPlatformAccessorySetupNFC setupNFC;
#endif
    HAPPlatformTCPStreamManager tcpStreamManager;
    HAPPlatformServiceDiscovery serviceDiscovery;
    HAPPlatformSoftwareAccessPoint softwareAccessPoint;
    HAPPlatformWiFiManager wiFiManager;
    HAPPlatformMFiHWAuth mfiHWAuth;
    HAPPlatformMFiTokenAuth mfiTokenAuth;
} platform;
```

- The platform variable contains those PAL services that need to be instantiated, and that are relevant for the given
accessory. They are initialized individually and will be used at runtime once the server has been started. In contrast,
the camera part of the PAL is not instantiated here, because it is not relevant for a light bulb. The random number
generator in the PAL is an example of a service that exists only as a single instance and thus needs no explicit
instantiation.
- Currently, concurrent *IP* and *BLE* operations are not supported, i.e., an accessory is either an *IP* or a *BLE*
accessory. However, concurrent *THREAD* and *BLE* operations are supported.
- All memory provided through arguments to `HAPAccessoryServerCreate` must stay valid until the accessory server has
been released (`HAPAccessoryServerRelease)`. Be careful to ensure that this condition holds if you call
`HAPAccessoryServerCreate` from a scope in which the arguments do not automatically stay valid until the accessory
server has been released.

``` Important::
    Important: The HAP Library runs as a single execution context (e.g., a thread) in one loop, and all HAP.h functions
    must be called from this execution context. The blocking function ``HAPPlatformRunLoopRun`` implements the run loop,
    which manages and dispatches I/O events and drives the scheduling of timer events. When calling a HAP.h function
    from another execution context, it must be scheduled by calling ``HAPPlatformRunLoopScheduleCallback``.

    **HAPPlatformRunLoopScheduleCallback is the function that is always safe to call from any execution context.**

    NOTE: Callbacks issued by the HAP Library, from ``HAPPlatformRunLoopRun`` are already on the ADK’s execution
    context.
```

Please refer to `__RunApplication` function in `Applications/AppBase.c` for a complete example of starting an
ADK Application.

### Terminate the Application

Normally, the HomeKit accessory server is never stopped as long as an accessory is powered. To stop it anyway, follow
this sequence of steps as illustrated in the LightBulb sample Application:

- Make sure that you are running on the ADK execution context, see: `HAPPlatformRunLoopScheduleCallback` in
`AdkStopApplication` in `Applications/AppBase.c`.
- Stop the accessory server, see: `HAPAccessoryServerStop` in `StopApplicationCallback` in `Applications/AppBase.c`.
- Wait for the accessory server to transition to the idle state. Then stop the run loop. See:
`HAPPlatformRunLoopStop` in `HandleUpdatedState` in `Applications/AppBase.c`.
- Release the accessory server, see: `HAPAccessoryServerRelease` in `Applications/AppBase.c`.
- Release the run loop, see: `HAPPlatformRunLoopRelease` in `DeinitializePlatform` in `Applications/AppBase.c`.

### Memory Requirements
Note that the HAP Library does not allocate any memory dynamically. Memory for the *BLE*, *IP* or *THREAD* stack is
provided in the form of static variables. Worst-case memory requirements depend on the number and size of the
characteristic values in the accessory database. In the case of HomeKit bridges, it also depends on the number of
bridged accessories. It may take a few runs with the *HomeKit Certification Assistant (HCA)* to find values that are
small but still large enough to handle all requests from a HomeKit controller.

In `AppBase.c`, the samples use HAP-defined default constants for IP buffer sizes and the like. Start with these
defaults, and then change the values as needed. These are the minimal values, appropriate for simple HomeKit accessories
like light bulbs or sensors:

- Adjust `kHAPIPSessionStorage_DefaultNumElements` to reduce the number of HomeKit connections that can be open
simultaneously.
- Adjust `kHAPIPSession_DefaultInboundBufferSize` to reduce the buffer space for messages coming from a HomeKit
controller (one buffer per connection).
- Adjust `kHAPIPSession_DefaultOutboundBufferSize` to reduce the buffer space for messages going to a HomeKit
controller (one buffer per connection).
- Adjust `kHAPIPSession_DefaultScratchBufferSize` to reduce the buffer size for various protocol operations
(one buffer per accessory/bridge).

If you replace one or more of the above values, be sure to replace all occurrences. Make sure that the number of
parallel HomeKit sessions that is supported is correctly configured. On a PAL that is derived from the POSIX PAL,
this means that `ipAccessoryServerStorage.numSessions` must have the same value as the option
`HAPPlatformTCPStreamManagerOptions.maxConcurrentTCPStreams`.

### Troubleshooting

- Make sure there is sufficient memory on the development platform—the system is built with debug logging enabled.
For example, this is done with make `BUILD_TYPE=Debug`. Adapt the makefiles accordingly by setting the `LOG_LEVEL`
compile time flag. The log will be written to the console.
- If there is an error in the log that says precondition failed, the log message also says what is expected in this
place. A precondition failure means that a function is called with illegal arguments or that the system is in an
erroneous state that is not acceptable for this function to run correctly. For example, a function may be called with a
null value for a non-nullable pointer parameter. The ADK systematically checks for precondition failures, which always
indicate programming errors (“contract violations”). If one is detected, the ADK is aborted immediately (*fail fast*)
so that the program error is not masked.
- If you randomly encounter errors, e.g., leading to ADK aborts: please check that all calls to the HAP Library occur
in the correct execution context. They must not occur from arbitrary threads.
- Make sure use of the Apple Authentication Coprocessor is not the cause of the failure, by disabling it
(temporarily during early development, not for a shipping product). You can do this with the build option
`USE_HW_AUTH=0`.
- If you use a PAL that is derived from the POSIX PAL and you get the Failed to allocate IP session error message in
the HAP log, then check that the option `HAPAccessoryServerOptions.ip.accessoryServerStorage.numSessions` has the same
value as the option `HAPPlatformTCPStreamManagerOptions.maxConcurrentTCPStreams`.
- If you get the Closing connection (inbound buffer too small) error message in the HAP log, increase the value of
`kHAPIPSession_DefaultInboundBufferSize`.
- For every characteristic in the accessory attribute database, make sure that the characteristic format matches the
characteristic structure, i.e., a `HAPUInt8Characteristic` must have `kHAPCharacteristicFormat_UInt8`.
- If there is an issue during firmware update testing, check that existing IIDs have remained the same as before the
firmware update. This is particularly important if a firmware update now uses the ADK and the ADK samples and their
IIDs are copied as is, without adapting them to their pre-ADK values.
- Pairing with HomeKit Accessory Tester tool and consulting its trace view for errors may assist with diagnostics.
- Make sure that the accessory attribute database is set up according to the profile definition of the latest HAP
specification. If this is not the case, the HomeKit controller may abort pairing.

## Persistent storage

Accessory logic, HAP Library and the PAL may all need to store some data in a persistent way. On Linux systems, this may
be in a file or in a trusted platform module. On microcontrollers, this may be stored directly in on-chip flash memory.

- Accessory logic, e.g., for the LightBulb profile, typically needs very little persistent storage. The other
applications such as Remote profile require several hundred KB for storing large configuration data structures. Such
application data can be stored using the ADK’s key-value store, or outside of it.
- The HAP Library uses the ADK's [KeyValue Store](_api_docs/file_PAL_HAPPlatformKeyValueStore.h.html) for storing
configuration data. The structure and storage format is not relevant for licensees and not documented in this guide.
- The PAL implements the key-value store. Besides the HAP Library and the accessory logic, PAL modules may also use the
key-value store. E.g., for implementing provisioning support (see also
[Accessory Setup](_api_docs/file_PAL_HAPPlatformAccessorySetup.h.html)). Other storage mechanisms may
also be used whenever appropriate. E.g., the POSIX PAL stores WAC configuration data in the same location where a Linux
system stores the Wi-Fi configuration data.

At least the data that the HAP Library stores persistently, but preferably also any accessory logic data and PAL data
that may be security-critical, should be stored in secure storage - as secure as the target platform supports.

## Firmware updates
- After a firmware update, the field firmwareVersion in the `HAPAccessory` object must be incremented by the accessory
logic. The remaining necessary state changes are automatically done by the HAP Library, e.g., incrementing the
config number (CN).
- An *IID* must remain the same throughout the lifetime of the accessory pairing. This means in particular across
firmware updates. If an *IID* is not used anymore after a firmware update, it must not be reused for anything else.
