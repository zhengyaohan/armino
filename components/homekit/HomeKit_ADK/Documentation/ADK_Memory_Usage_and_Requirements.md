Memory Requirements
===================

This document provides information on how memory is used by the ADK, as well as guidance for fine tuning memory consumption.  The ADK is designed to be generic, and as such does not provide fine tuned memory solutions for each accessory type.  This doc describes ADK requirements for a basic profile such as `Lightbulb` and provides tips to allow vendors to fine tune their individual solutions.

 - [Thread](#thread)
 - [BLE](#ble)
 - [IP](#ip)

## Thread
This section describes the memory requirements of the Nordic implementation of Thread.

### Memory Requirements

#### Release Configuration
NOTE: The following requirements are for Full Thread Devices:

| Memory Type   | Size (KB) | Notes    |
| :------------:| :--------:|:--------:|
| RAM           | `128.35`  |Further breakdown available in the table below|
| Flash         | `427`     ||
| Secure Storage| `4`       | Writable non-volatile storage (flash or EEPROM)|

| RAM Section   | Size (KB) |
| :------------:| :--------:|
| Stack         | `8`       |
| Heap          | `8`       |
| Mem Mgr Heap* | `40.56`   |
| .bss          | `71.79`   |
| **TOTAL**     |**128.35** |

Largest contributors to BSS:

| Symbol       | Size (KB)  |
| :------------:| :--------:|
| `otInstanceRaw` | `35.375`    |
| `threadSessions`| `4`         |
| `nrf_rx_buffers`| `2.02`      |
| `APP_SCHED_BUF` | `2.02`      |

#### Debug Configuration

| Memory Type   | Size (KB) | Notes    |
| :------------:| :--------:|:--------:|
| RAM           | `146.93`  |Further breakdown available in the table below|
| Flash         | `636`     ||
| Secure Storage| `4`       | Writable non-volatile storage (flash or EEPROM)|

| RAM Section   | Size (KB) |
| :------------:| :--------:|
| Stack         | `8`         |
| Heap          | `8`         |
| Mem Mgr Heap* | `40.56`     |
| .bss          | `91.26`     |
| **TOTAL**     | **146.93**|

Largest contributors to BSS:

| Symbol       | Size (KB)  |
| :------------:| :--------:|
| `otInstanceRaw` | `35.375`    |
| `acUpBuffer`    | `13.67`     |
| `debugCommand`  | `8`         |
| `threadSessions`| `4`         |

**\*** The Nordic Mem Mgr Heap is statically allocated memory and is actually part of the .bss.

### ADK Memory Usage

The ADK utilizes an 8k heap and an 8k stack for local variables and function calls.  The ADK avoids allocating data from the heap when possible, preferring to statically allocate its buffers.  Statically allocated ADK buffers that can be tuned include:

* **threadSessions**:  The size of this buffer is controlled by `kAppConfig_NumThreadSessions` which determines the maximum number of simultaneous Thread sessions an accessory can support.  Default is the minimum allowed by spec: 8.
* **keyValueStoreBytes**:  Declared in function `HAPPlatformSetupInitKeyValueStore`.  This buffer is used to build a queue of keyValueStore commands when interacting with the keyValueStore.  Resize according to kvs need.
* **MAX_NUM_COAP_REQUESTS**:  This parameter determines how many CoAP messages can be "in Flight" between an accessory and the controller. ADK tracks messages "In Flight" to determine how best to process failed responses.  ADK Default: 8.
* **kAppConfig_NumAccessoryServerSessionStorageBytes**: This parameter determines how many bytes are provided to Thread Session Storage.  Session storage is used to store outgoing Event notifications and handle Timed Writes.  The number of characteristics that support these features will determine the minimum safe size for this buffer.
* **APP_SCHED_BUF**: Used to store SoftDevice application events.  The size is controlled by `SCHED_QUEUE_SIZE`.

### OpenThread Memory Usage

The OpenThread library's memory footprint depends a great deal on compile time configuration.  The required configuration depends on the requirements of the accessory, and as such must be tuned by the accessory developer / vendor.  This section describes ADK defaults and provides tips on how to tune OpenThread for your accessory.

The OpenThread library is built with any number of [Build Switches](https://github.com/openthread/openthread/blob/master/examples/common-switches.mk).  Each of these switches have their own memory requirements and may or may not be necessary for your accessory.

In addition to Build Switches, the following compile time parameters have a significant impact on OpenThread's memory footprint:

* [OPENTHREAD_CONFIG_MLE_MAX_CHILDREN](https://github.com/openthread/openthread/blob/2b9e8facba4e30fd5c28cc19e4f870abcc08cdc9/src/core/config/mle.h#L58).  This compile time parameter determines how many children the accessory can support.  Should be set to 0 for sleepy devices.  Default is 32, which is likely higher than most FTDs will want to support.
* [OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS](https://github.com/openthread/openthread/blob/2b9e8facba4e30fd5c28cc19e4f870abcc08cdc9/src/core/config/openthread-core-default-config.h#L137).  Determines how many message buffers a node needs.
* [OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES](https://github.com/openthread/openthread/blob/2b9e8facba4e30fd5c28cc19e4f870abcc08cdc9/src/core/config/tmf.h#L45).  EID to RLOC cache entries.
* [OPENTHREAD_CONFIG_COAP_SERVER_MAX_CACHED_RESPONSES](https://github.com/openthread/openthread/blob/2b9e8facba4e30fd5c28cc19e4f870abcc08cdc9/src/core/config/coap.h#L46) The Number of CoAP responses to save for retries
* [All Diagnostic Parameters](https://github.com/openthread/openthread/blob/master/src/core/config/diag.h)

ADK builds with the following OpenThread build switches:

* `BORDER_AGENT=1`
* `BORDER_ROUTER=1`
* `COAP=1 `
* `COMMISSIONER=1`
* `DISABLE_BUILTIN_MBEDTLS=1`
* `DNS_CLIENT=1`
* `DIAGNOSTIC=1`
* `EXTERNAL_HEAP=1`
* `IP6_FRAGM=1`
* `JOINER=1`
* `LINK_RAW=1`
* `MAC_FILTER=1`
* `MTD_NETDIAG=1`
* `SERVICE=1`
* `UDP_FORWARD=1`
* `ECDSA=1`
* `SNTP_CLIENT=1`
* `COAPS=1`
* `DHCP6_SERVER=1`
* `DHCP6_CLIENT=1`
* `CHILD_SUPERVISION=1`
* `JAM_DETECTION=1`

Many of these features may be unnecessary for your application.  An accessory that is not a Border Router does not need `BORDER_AGENT`, `BORDER_ROUTER`, or `COMMISSIONER` for example.

### Nordic Stack and Memory Manager

In addition to a standard stack and heap, Nordic SoftDevices provide a [Memory Manager](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.0.0%2Fgroup__mem__manager.html).  This memory manager statically allocates a chunk of memory and allows applications allocate from it as though it were standard heap. The functions `nrf_calloc` and `nrf_free` may be used to allocate memory from this managed heap.

The memory manager block sizes are defined in `Build/nRF52/sdk_config.h` and should be tailored by vendors directly.  OpenThread's requirements of the memory manager are summarized as follows:

| Block Size (Bytes)| Num Blocks   | Total Per Block|
| :----------------:| :-----------:| :-------------:|
| `4`               | `32`         |  `128`         |
| `16`              | `64`         |  `1024`        |
| `2`               | `240`        |  `480`         |
| `4`               | `1280`       |  `5120`        |
| `2`               | `2908`       |  `5816`        |
|**TOTAL (Bytes)**  |              | **12568**      |

This is a generalization.  Some OT features, such as Joiner, may require a different larger block allocation.

In order to support all devices, the values provided by ADK are considerably larger. ADK provides the current allocation, which should be tuned by individual accessory developers according to need, as what is provided by ADK is considerably larger than necessary

| Block Size (Bytes)| Num Blocks   | Total Per Block|
| :----------------:| :-----------:| :-------------:|
| `64`              | `64`         |  `4096`        |
| `16`              | `256`        |  `4096`        |
| `4`               | `512`        |  `2048`        |
| `5`               | `2816`       |  `14080`       |
| `5`               | `3444`       |  `17220`       |
|**TOTAL (Bytes)**  |              | **41540**      |

## BLE

This section describes the memory requirements of the Nordic implementation of BLE Protocol for `Lightbulb` sample application.

### Memory Requirements

#### Release Configuration

| Memory  Type  | Size (KB)   | Notes    |
| :------------:| :----------:|:--------:|
| RAM           | `29.11`     |Further breakdown available in the table below|
| Flash         | `95.84`     ||
| Secure Storage| `4`         | Writable non-volatile storage (flash or EEPROM)|

| RAM Section   | Size (KB)   |
| :------------:| :----------:|
| Stack         | `8`         |
| Heap          | `8`         |
| .bss          | `13.11`     |
| **TOTAL (KB)** |**29.11**    |

Largest contributors to BSS:

| Symbol              | Size (KB)   |
| :------------------:| :----------:|
| `APP_SCHED_BUF`     | `2.02`      |
| `procedureBytes`    | `2`         |
| `keyValueStoreBytes`| `2`         |
| `accessoryServer`   | `1.88`      |

#### Debug Configuration

| Memory  Type  | Size (KB)   | Notes    |
| :------------:| :----------:|:--------:|
| RAM           | `51.08`     |Further breakdown available in the table below|
| Flash         | `249`       ||
| Secure Storage| `4`         | Writable non-volatile storage (flash or EEPROM)|

| RAM Section   | Size (KB)   |
| :------------:| :----------:|
| Stack         | `8`         |
| Heap          | `8`         |
| .bss          | `35.08`     |
| **TOTAL (KB)**| **51.08** |

Largest contributors to BSS:

| Symbol          | Size (KB)  |
| :--------------:| :---------:|
| `acUpBuffer`    | `13.67`    |
| `debugCommand`  | `8`        |
| `APP_SCHED_BUF` | `2.02`     |
| `procedureBytes`| `2`        |

### ADK Memory Usage

The ADK utilizes an 8k heap and an 8k stack for local variables and function calls.  The ADK avoids allocating data from the heap when possible, preferring to statically allocate its buffers.  Statically allocated ADK buffers that can be tuned include:

* **keyValueStoreBytes**.  Declared in function `HAPPlatformSetupInitKeyValueStore`.  This buffer is used to build a queue of keyValueStore commands when interacting with the keyValueStore.  Resize according to kvs need.
* **APP_SCHED_BUF** Used to store SoftDevice application events.  The size is controlled by `SCHED_QUEUE_SIZE`.

### MBedTLS Crypto Requirements

In addition to a standard stack and heap, Nordic SoftDevices provide a [Memory Manager](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.0.0%2Fgroup__mem__manager.html).  This memory manager statically allocates a chunk of memory and allows applications allocate from it as though it were standard heap. The functions `nrf_calloc` and `nrf_free` may be used to allocate memory from this managed heap.

MBedTLS must be given a pointer to `nrf_calloc` and `nrf_free` via the `mbedtls_platform_set_calloc_free`.  This necessitates setting up the NRF Memory manager's.

The memory manager block sizes are defined in `Build/nRF52/sdk_config.h` and should be tailored by vendors directly.

MbedTLS crypto requires the following minimum block sizes:

| Block Size (Bytes)| Num Blocks   | Total Per Block|
| :----------------:| :-----------:| :-------------:|
| `16`              | `16`         |  `256`         |
| `8`               | `64`         |  `512`         |
| `8`               | `256`        |  `2048`        |
| `4`               | `512`        |  `2048`        |
| `6`               | `1024`       |  `6144`        |
| `2`               | `3444`       |  `6888`        |
| **TOTAL (Bytes)** |              | **17896**      |

In order to support all devices, the values provided by ADK are considerably larger. ADK provides the current allocation, which should be tuned by individual accessory developers according to need, as what is provided by ADK is considerably larger than necessary

| Block Size (Bytes)| Num Blocks   | Total Per Block|
| :----------------:| :-----------:| :-------------:|
| `64`              | `64`         |  `4096`        |
| `16`              | `256`        |  `4096`        |
| `4`               | `512`        |  `2048`        |
| `5`               | `2816`       |  `14080`       |
| `5`               | `3444`       |  `17220`       |
| **TOTAL (Bytes)** |              | **41540**      |

Note that these values may increase depending on other memory requirements.  Vendors should tune these values according to need.

## IP

This section describes the memory requirements of the Raspberry Pi implementation of IP Protocol for the `Lightbulb`
sample application. Note that memory requirements vary based upon the HomeKit profile and features an accessory
incorporates.

### Memory Requirements

#### Release Configuration

| Memory  Type  | Size (KB)   | Notes    |
| :------------:| :----------:|:--------:|
| RAM           | `1178.3`    | Further breakdown available in the table below
| Flash         | `135.8`     | Code space, read-only and initialized data
| Secure Storage| `4`         | Writable non-volatile storage (flash or EEPROM)

| RAM Section   | Size (KB)   |
| :------------:| :----------:|
| .bss          | `1175.2`    |
| .data         | `3.1`       |
| **TOTAL (KB)**|**1178.3**   |

Largest contributors to BSS:

| Symbol                | Size (KB)   |
| :--------------------:| :----------:|
| `ipOutboundBuffers`   | `557`       |
| `ipInboundBuffers`    | `557`       |
| `ipScratchBuffer`     | `32`        |
| `ipSessions`          | `12`        |

#### Debug Configuration

| Memory  Type  | Size (KB)   | Notes    |
| :------------:| :----------:|:--------:|
| RAM           | `1194.3`    | Further breakdown available in the table below
| Flash         | `1633.6`    | Code space, read-only and initialized data
| Secure Storage| `4`         | Writable non-volatile storage (flash or EEPROM)

| RAM Section   | Size (KB)   |
| :------------:| :----------:|
| .bss          | `1185.8`    |
| .data         | `8.5`       |
| **TOTAL (KB)**|**1194.3**   |

Largest contributors to BSS:

| Symbol                | Size (KB)   |
| :--------------------:| :----------:|
| `ipOutboundBuffers`   | `557`       |
| `ipInboundBuffers`    | `557`       |
| `ipScratchBuffer`     | `32`        |
| `ipSessions`          | `12`        |

Debug builds include precondition and assert checks, which are disabled in release builds. Release builds also
disable logging, resulting in a much smaller flash footprint.

### ADK Memory Usage

By default, the ADK HAP implementation uses static memory allocation. As seen above, the majority of RAM usage
corresponds to buffers whose sizing is determined as follows:

* **ipSessions**: IP session storage. Controlled by `kHAPIPSessionStorage_DefaultNumElements` and limits the
maximum number of simultaneous IP sessions an accessory can support. Default is 17.
* **ipInboundBuffers**: Buffers for controller-to-accessory messages. Controlled by the number of IP sessions
and `kHAPIPSession_DefaultInboundBufferSize`. Default is 32 KB per session.
* **ipOutboundBuffers**: Buffers for accessory-to-controller messages. Controlled by the number of IP sessions
and `kHAPIPSession_DefaultOutboundBufferSize`. Default is 32 KB per session.

Note that ADK memory usage does not incorporate OS resources, platform dependencies such as the IP stack or shared
libraries, nor the stack allocated for the context in which the ADK executes.

### HAP_DYNAMIC_MEMORY_ALLOCATION

The `HAP_DYNAMIC_MEMORY_ALLOCATION` build flag may be used to dynamically allocate the inbound and outbound buffers
as well as event notification objects based upon session activity to minimize the idle memory usage. If enabled,
the platform must provide memory allocation, reallocation, and deallocation functions. This has a large impact on
the .bss section size, although peak RAM usage is dependent upon the number of controllers simultaneously exchanging
data with the accessory.

#### Release Configuration (`HAP_DYNAMIC_MEMORY_ALLOCATION=1`)

| Memory  Type  | Size (KB)   | Notes    |
| :------------:| :----------:|:--------:|
| RAM           | `53.8  `    | Further breakdown available in the table below
| Flash         | `138.4`     | Code space, read-only and initialized data
| Secure Storage| `4`         | Writable non-volatile storage (flash or EEPROM)

| RAM Section   | Size (KB)   |
| :------------:| :----------:|
| .bss          | `50.6`      |
| .data         | `3.2`       |
| **TOTAL (KB)**|**53.8**     |

Largest contributors to BSS:

| Symbol                | Size (KB)   |
| :--------------------:| :----------:|
| `ipScratchBuffer`     | `32`        |
| `ipSessions`          | `12`        |
| `accessoryServer`     | `2.3`       |
| `ipWriteContexts`     | `1.2`       |

#### Debug Configuration (`HAP_DYNAMIC_MEMORY_ALLOCATION=1`)

| Memory  Type  | Size (KB)   | Notes    |
| :------------:| :----------:|:--------:|
| RAM           | `68.5`      | Further breakdown available in the table below
| Flash         | `1645.5`    | Code space, read-only and initialized data
| Secure Storage| `4`         | Writable non-volatile storage (flash or EEPROM)

| RAM Section   | Size (KB)   |
| :------------:| :----------:|
| .bss          | `60`        |
| .data         | `8.5`       |
| **TOTAL (KB)**|**68.5**     |

Largest contributors to BSS:

| Symbol                | Size (KB)   |
| :--------------------:| :----------:|
| `ipScratchBuffer`     | `32`        |
| `ipSessions`          | `12`        |
| `debugCommand`        | `8`         |
| `accessoryServer`     | `2.3`       |
