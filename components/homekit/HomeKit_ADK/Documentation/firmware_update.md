Firmware Update Service
=======================

## Overview
This document describes the firmware update feature provided by the ADK and how to integrate it into a HomeKit
accessory. It was developed in accordance with the HomeKit Accessory Protocol Firmware Update specification and the
Unified Accessory Restore Protocol (UARP) development guide.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK. Once all the
prerequisites are installed for your platform, run the following to compile ADK with firmware update feature for IP
accessories with TCP support:

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin HAP_FIRMWARE_UPDATE=1

      For BLE:

      .. code-block:: bash

       make TARGET=Darwin PROTOCOLS=BLE HAP_HDS_TRANSPORT_OVER_HAP=1 HAP_FIRMWARE_UPDATE=1

      For Thread:

      .. code-block:: bash

       make TARGET=Darwin PROTOCOLS=THREAD HAP_HDS_TRANSPORT_OVER_HAP=1 HAP_FIRMWARE_UPDATE=1

   .. group-tab:: Linux

      .. code-block:: bash

       make TARGET=Linux HAP_FIRMWARE_UPDATE=1

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       make TARGET=Raspi HAP_FIRMWARE_UPDATE=1

      For BLE:

      .. code-block:: bash

       make TARGET=Raspi PROTOCOLS=BLE HAP_HDS_TRANSPORT_OVER_HAP=1 HAP_FIRMWARE_UPDATE=1

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       make TARGET=nRF52 HAP_HDS_TRANSPORT_OVER_HAP=1 HAP_FIRMWARE_UPDATE=1

      For Thread:

      .. code-block:: bash

       make TARGET=nRF52 PROTOCOLS=THREAD HAP_HDS_TRANSPORT_OVER_HAP=1 HAP_FIRMWARE_UPDATE=1

```

### Run
Please follow instructions at [Running an ADK Application](getting_started.html#step-5-running-an-adk-application) to
run an ADK sample application.

## Implementation Details

The steps required to integrate the ADK firmware update feature into a HomeKit accessory are as follows:
- Add platform support, if needed, by adding `PAL/<platform>/HAPPlatformUARP.<h|c>`.
- Include the firmware update service in the accessory's services list.
- Include the HDS transport management service in the accessory's services list.
- Initialize HDS with support for UARP.
- Initialize the UARP and firmware update modules.
- Using `Applications/Common/Helper/FirmwareUpdate.c` as a starting point, implement the HAP firmware update state
  callback and the UARP accessory callbacks, including appropriate image staging and apply routines and corresponding
  state tracking/notifications.
- Enable the feature with the `HAP_FIRMWARE_UPDATE` build flag.
- Tune `kUARPDataChunkSize`, if needed, based on memory and performance tradeoff.

### Additional Information
Support for firmware updates directly through HomeKit minimizes the effort required by accessory vendors to support
updates and improves the user experience. The ADK implements the necessary framework for initiating a firmware update
and receiving the new firmware image onto the device. The accessory is required to stage the image into memory and
implement an apply routine as appropriate based upon the system design and underlying platform.

####  Unified Accessory Restore Protocol (UARP)
UARP is the protocol over which iOS controllers may notify an accessory of an available firmware update, the update
image is transferred to the device, and apply requests are issued. The ADK integrates the UARP accessory library
referenced in the UARP development guide and provides an API for accessory vendors to simplify integration of the
firmware update feature with the ADK.

#### SuperBinary Image Format
UARP relies on the SuperBinary Image Format, which is a wrapper around one or more firmware images or assets. It
consists of an overall header, payload header(s), and payload(s). The SuperBinary header and payload header(s) may
have associated metadata which is meant for the accessory to consume and perform operations which the controller would
not know how to interpret or process. More information on the SuperBinary format be found in *Section 3 - SuperBinary*
of the UARP development guide.

#### HomeKit Data Stream (HDS)
HDS is a message protocol built on a bidirectional byte stream. HDS supports various higher-level protocols, including
the stream protocol, which provides a generic data pipe for UARP messages between the iOS controller and accessory.
HDS may be run over TCP or HAP itself, which allows firmware updates to be supported on all underlying HAP transports.
Additional information regarding transport-specific HDS requirements is available in *Section 9 - HomeKit Data Stream*
of *HomeKit Accessory Protocol Specification*.

```sh

    +-----------------------+
    |          UARP         |
    +-----------------------+
    |    Stream Protocol    |
    +-----------------------+
    |    HDS Dispatcher     |
    +-----------------------+
    |          HDS          |
    +-----------------------+
    |  Transport (TCP/HAP)  |
    +-----------------------+
```

#### HAP Firmware Update Service
The service consists of two required characteristics:
- Firmware Update Readiness
- Firmware Update Status

These characteristics allow iOS controllers to query state information from the accessory which is outside the scope of
UARP or is required when an HDS session is not active. The accessory is required to maintain this state which will be
retrieved by the HAP module on characteristic reads. For characteristics which support notify permissions, the accessory
is required to initiate the notification when relevant state changes via `HAPAccessoryServerRaiseEvent()`.

Requirements regarding state transitions and accessory expectations are provided in *Section 2.1 - Accessory Firmware
Update Design* of the HAP specification.

### HAP Implementation
- *HAP/HAP+API.h*
  - Defines the accessory firmware update state structure and corresponding state retrieval callback which must be
    implemented by the accessory.

  ```c
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
  ```

  ```c
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
              HAPAccessoryServerRef* server,
              const HAPAccessory* accessory,
              HAPAccessoryFirmwareUpdateState* accessoryState,
              void* _Nullable context);
  } firmwareUpdate;
  ```

### PAL Implementation
- *PAL/*/HAPPlatformUARP.h*
  - Provides platform-specific includes for byte order conversion routines required by the UARP library.

- *PAL/*/HAPPlatformUARP.c*
  - Implements the ADK platform interface required by the UARP library, including logging and byte order conversion.
    Note that an implementation is required per-platform to compile, but all implementations symlink to the POSIX
    implementation.

### App Implementation
- *Applications/Common/Helper/UARP.h*
  - Defines the data chunk size used when pulling an image. This also corresponds to the size of the buffer the
    accessory is expected to consume during staging.
  - Details the accessory UARP API. This API consists of callbacks for handling UARP events which must be implemented by
    the accessory as well as functions for initialization and control of the staging flow.

  ```c
  /**
   * UARP callbacks for application handling.
   */
  typedef struct {
      /**
       * The callback used to notify the accessory of an asset offering.
       *
       * @param      server              Accessory server.
       * @param      accessory           The accessory that provides the service.
       * @param      assetVersion        Asset version.
       * @param      assetTag            Asset tag identifier.
       * @param      assetLength         Total asset length.
       * @param      assetNumPayloads    Number of payloads in the asset.
       * @param[out] shouldAccept        Accessory indicator to accept or deny the asset.
       */
      void (*assetOffered)(
              HAPAccessoryServer* server,
              const HAPAccessory* accessory,
              UARPVersion* assetVersion,
              uint32_t assetTag,
              uint32_t assetLength,
              uint16_t assetNumPayloads,
              bool* shouldAccept);

      /**
       * The callback used to notify the accessory of an asset metadata TLV for processing.
       *
       * @param      server          Accessory server.
       * @param      accessory       The accessory that provides the service.
       * @param      tlvType         Type field of TLV.
       * @param      tlvLength       Length field of TLV.
       * @param      tlvValue        Buffer holding TLV value field.
       */
      void (*assetMetadataTLV)(
              HAPAccessoryServer* server,
              const HAPAccessory* accessory,
              uint32_t tlvType,
              uint32_t tlvLength,
              uint8_t* tlvValue);

      /**
       * The callback used to notify the accessory of asset metadata processing completion.
       *
       * @param      server          Accessory server.
       * @param      accessory       The accessory that provides the service.
       */
      void (*assetMetadataComplete)(HAPAccessoryServer* server, const HAPAccessory* accessory);

      /**
       * The callback used to provide the accessory with payload header information.
       *
       * @param      server               Accessory server.
       * @param      accessory            The accessory that provides the service.
       * @param      payloadVersion       Payload version.
       * @param      payloadTag           Payload tag identifier.
       * @param      payloadLength        Payload length.
       */
      void (*payloadReady)(
              HAPAccessoryServer* server,
              const HAPAccessory* accessory,
              UARPVersion* payloadVersion,
              uint32_t payloadTag,
              uint32_t payloadLength);

      /**
       * The callback used to notify the accessory of a payload metadata TLV for processing.
       *
       * @param      server          Accessory server.
       * @param      accessory       The accessory that provides the service.
       * @param      tlvType         Type field of TLV.
       * @param      tlvLength       Length field of TLV.
       * @param      tlvValue        Buffer holding TLV value field.
       */
      void (*payloadMetadataTLV)(
              HAPAccessoryServer* server,
              const HAPAccessory* accessory,
              uint32_t tlvType,
              uint32_t tlvLength,
              uint8_t* tlvValue);

      /**
       * The callback used to notify the accessory of payload metadata processing completion.
       *
       * @param      server          Accessory server.
       * @param      accessory       The accessory that provides the service.
       */
      void (*payloadMetadataComplete)(HAPAccessoryServer* server, const HAPAccessory* accessory);

      /**
       * The callback used to notify the accessory of payload data.
       *
       * @param      server          Accessory server.
       * @param      accessory       The accessory that provides the service.
       * @param      payloadTag      Payload tag identifier.
       * @param      offset          Offset for this data chunk.
       * @param      buffer          Buffer holding the data.
       * @param      bufferLength    Length of data buffer.
       */
      void (*payloadData)(
              HAPAccessoryServer* server,
              const HAPAccessory* accessory,
              uint32_t payloadTag,
              uint32_t offset,
              uint8_t* buffer,
              uint32_t bufferLength);

      /**
       * The callback used to notify the accessory of payload processing completion.
       *
       * @param      server          Accessory server.
       * @param      accessory       The accessory that provides the service.
       */
      void (*payloadDataComplete)(HAPAccessoryServer* server, const HAPAccessory* accessory);

      /**
       * The callback used to notify the accessory of a change in asset state.
       *
       * @param      server    Accessory server.
       * @param      accessory The accessory that provides the service.
       * @param      state     Asset state change.
       */
      void (*assetStateChange)(HAPAccessoryServer* server, const HAPAccessory* accessory, UARPAssetStateChange state);

      /**
       * The callback used to inform the accessory of an apply staged assets request.
       * It is expected that accepting the apply request will result in the accessory being reachable on the new firmware
       * version within the update duration time provided by the firmware update HAP profile.
       *
       * @param      server            Accessory server.
       * @param      accessory         The accessory that provides the service.
       * @param[out] requestRefused    Accessory indicator for refusing the apply request.
       */
      void (*applyStagedAssets)(HAPAccessoryServer* server, const HAPAccessory* accessory, bool* requestRefused);

      /**
       * The callback used to retrieve the last accessory error.
       *
       * @param      server             Accessory server.
       * @param      accessory          The accessory that provides the service.
       * @param      lastErrorAction    Accessory last error and associated action.
       */
      void (*retrieveLastError)(
              HAPAccessoryServer* server,
              const HAPAccessory* accessory,
              UARPLastErrorAction* lastErrorAction);
  } UARPAccessoryCallbacks;
  ```

  ```c
  /**
   * Initialize UARP.
   *
   * @param      server       HAP accessory server.
   * @param      accessory    HAP accessory containing accessory information.
   */
  void UARPInitialize(HAPAccessoryServer* server, const HAPAccessory* accessory);

  /**
   * Register for firmware assets.
   *
   * @param      callbacks                Callbacks for firmware asset events.
   * @param      stagedFirmwareVersion    Version of currently staged firmware.
   */
  HAP_RESULT_USE_CHECK
  HAPError UARPRegisterFirmwareAsset(
          const UARPAccessoryFirmwareAssetCallbacks* callbacks,
          UARPVersion* stagedFirmwareVersion);

  /**
   * Request a payload from the SuperBinary by payload index.
   * Payloads are indexed starting from 0.
   * It is expected the payload processing order will be built into the accessory's update scheme or will be derived
   * from the SuperBinary metadata.
   *
   * @param      payloadIndex    Payload index to begin pulling.
   *
   * @return kHAPError_None            If successful.
   * @return kHAPError_InvalidState    If there is no active asset.
   * @return kHAPError_InvalidData     If the payload index is invalid.
   * @return kHAPError_Unknown         If the UARP library is unable to process the request.
   */
  HAP_RESULT_USE_CHECK
  HAPError UARPRequestFirmwareAssetPayloadByIndex(uint32_t payloadIndex);

  /**
   * Changes the offset into the active payload.
   *
   * @param      payloadOffset   Relative offset into the payload.
   *
   * @return kHAPError_None            If successful.
   * @return kHAPError_InvalidState    If there is no active asset.
   * @return kHAPError_InvalidData     If the payload or payload offset is invalid.
   * @return kHAPError_Unknown         If the UARP library is unable to process the request.
   */
  HAP_RESULT_USE_CHECK
  HAPError UARPSetFirmwareAssetPayloadDataOffset(uint32_t payloadOffset);

  /**
   * Indicate the asset staging has been completed.
   *
   * @param      stagedVersion   Version of the update staged by the accessory.
   *
   * @return kHAPError_None            If successful.
   * @return kHAPError_InvalidState    If there is no active asset.
   * @return kHAPError_Unknown         If the UARP library was unable to process the request.
   */
  HAP_RESULT_USE_CHECK
  HAPError UARPFirmwareAssetFullyStaged(UARPVersion* stagedVersion);

  /**
   * Requests a change to the asset staging state.
   *
   * @param      state    Requested asset staging state change
   *
   * @return kHAPError_None            If successful.
   * @return kHAPError_InvalidState    If there is no active asset.
   * @return kHAPError_Unknown         If the UARP library is unable to process the request.
   */
  HAP_RESULT_USE_CHECK
  HAPError UARPRequestFirmwareAssetStagingStateChange(UARPAssetStagingStateChangeRequest state);

  /**
   * Checks if the UARP version is zero.
   *
   * @param      version    UARP Version.
   *
   * @return true     If all fields of the version are zero.
   * @return false    If at least one version field is non-zero.
   */
  bool UARPIsVersionZero(const UARPVersion* version);

  /**
   * Checks if the UARP versions are equal.
   *
   * @param      version1    The 1st UARP Version for comparison.
   * @param      version2    The 2nd UARP Version for comparison.
   *
   * @return true     If all fields of the version are equal.
   * @return false    If at least one version field is not equal.
   */
  bool UARPAreVersionsEqual(const UARPVersion* version1, const UARPVersion* version2);
  ```

- *Applications/Common/Helper/UARP.c*
  - Implements the accessory API interface to the UARP library.
  - Implements the binding layer between HDS and the UARP library.
  - Manages statically-allocated buffers to satisfy dynamic memory requests from the UARP library.

- *Applications/Common/Helper/FirmwareUpdate.c*
  - Provides a sample implementation of the firmware update accessory interface, including UARP accessory callbacks and
    basic state tracking to satisfy the HAP firmware update profile.
  - Provides an option for persisting the staged firmware version in the key value store (KVS).
  - Writes the firmware update payload data to a local file on POSIX based accessories. This data is saved in the
    location specified by the value in `kFirmwareAssetFile`.
  - It does not provide an apply routine other than changing the firmware version reported by the accessory after a
    simulated apply window. For accessories, it is expected a reboot will be needed to boot into the new firmware.
    `HAPAccessoryServerStop()` can be called upon receiving an apply request to gracefully shut down the accessory
    server prior to rebooting.
  - It provides limited support for the Firmware Update Readiness characteristic, which is dependent upon accessory-
    specific decisions.
  - This file may be used as a reference implementation for integrating the firmware update feature into an accessory.

- *Applications/*/App.c*
  - All sample applications include support for firmware updates by:
      - Including the firmware update and data stream transport management services in its services list.
      - Initializing HDS and the firmware update module.

### Example UARP callback flow
If a controller determines it has an asset to offer and the accessory indicates it is ready to stage an update (via
the Firmware Update Readiness characteristic), the controller will set up an HDS session and proceed to offer the
asset. The ADK will validate the SuperBinary version is greater than that of the firmware currently running on the
device and there is not already a staged firmware with an equal or greater version. The asset offer will then be
forwarded to the accessory via `assetOffered()` for final confirmation to accept the asset. Should the accessory
choose not to accept the asset, the offer will be declined.

After an asset has been accepted to initiate staging, the SuperBinary header will be pulled first. If there is metadata
associated with the SuperBinary header, it will be pulled and each TLV will be provided to the accessory via
`assetMetadataTLV()`.

After all SuperBinary metadata TLVs have been processed, the accessory will be notified via `assetMetadataComplete()`.
The accessory must set the payload to pull via `UARPRequestPayloadByIndex()`. This allows an accessory to pull a subset
of payloads included in the SuperBinary, process payloads out-of-order, etc. It is expected this payload processing
order will be built into the accessory's update scheme or will be derived from the SuperBinary metadata. Alternatively,
the payload headers may be iterated if logic is dependent upon payload information (version, tag) or payload metadata.

After the accessory has requested a payload, its corresponding header is pulled. Payload information will be provided
to the accessory via the `payloadReady()` callback once the header has been received and processed. After the callback
returns, any metadata associated with the payload will be pulled first, with each TLV provided to the accessory via
`payloadMetadataTLV()`. After all payload metadata TLVs have been processed, the accessory will be notified via
`payloadMetadataComplete()`. If payload information or metadata requires iterating payload headers and/or associated
metadata, a new payload can be requested using `UARPRequestPayloadByIndex()` in the `payloadReady()` or
`payloadMetadataComplete()` callbacks.

The accessory can also optionally set the payload data offset in the `payloadMetadataComplete()` callback. By
default, the accessory receives payload data from the controller starting at offset 0. By calling
`UARPSetPayloadDataOffset()`, the offset from which the accessory starts receiving payload data can be altered.
This may be useful in cases where the accessory resets or loses power during staging. Instead of re-transmitting the
entire firmware asset, the accessory can resume from its last known good offset. More information is available below
in [Transfer Concepts](#transfer-concepts).

Once `payloadMetadataComplete()` returns, the payload itself will be pulled based on the chunk size
`kUARPDataChunkSize`. Each payload chunk will be provided to the accessory via `payloadData()`.

After the entire payload has been provided, the accessory will be notified via `payloadDataComplete()`. At this point
the accessory may initiate pulling of another payload via `UARPRequestPayloadByIndex()` or indicate the asset has been
staged via `UARPAssetFullyStaged()`.

When a controller determines a staged update should be applied and the accessory indicates it is ready to do so
(via the Firmware Update Readiness characteristic), the accessory will be notified via `applyStagedAssets()`. If the
accessory's readiness changes and it is no longer able to apply the update in accordance with *Section 2.1.2 - Applying
Update* of the HAP spec, it must refuse the request.

During staging, the controller may explicitly pause/resume the transfer. The accessory will be notified via
`assetStateChange()` with state `kUARPAssetStateChange_StagingPaused` and `kUARPAssetStateChange_StagingResumed`,
respectively. In addition, staging will also be paused if an HDS disconnect occurs or if a transfer stall condition is
detected due to timeout of a data request. In such scenarios, the accessory is notified of the pause such that the HAP
profile may be updated accordingly. When a controller connects and an offer is received such that staging may be
resumed, the accessory is again notified accordingly. The controller may also rescind a previously accepted asset,
either during staging or after staging has been completed. This event will be reflected by
`kUARPAssetStateChange_AssetRescinded` and the accessory should clear any partially or fully staged asset accordingly.
Should the UARP stack detect issues with asset formatting while processing the SuperBinary or payload headers, the
accessory will be notified with an event type of `kUARPAssetStateChange_AssetCorrupt` and the transfer will be
abandoned.

If an accessory's staging readiness changes during the course of staging, it may alert the controller by setting a
relevant "Staging Not Ready Reason" bit in the HAP profile (reference `FwUpSetHAPProfileStagingNotReadyReason`).
This will generate a notification to the controller and the transfer will be paused. Upon clearing the bit, the
controller will resume staging.

### Transfer Concepts

#### Controller disconnect
If an asset is accepted and a controller disconnect occurs prior to the accessory indicating the asset has been fully
processed, the asset will be "orphaned". The accessory will be notified staging has been paused and the asset state
will be maintained within the UARP stack. If the controller re-connects or a secondary controller connects and offers
the same asset version, the transfer will be resumed. Note that the asset state maintained to resume a transfer is
stored in initialized RAM, meaning the UARP stack does not support resumes across resets.

#### Accessory-driven resume
If a reset or power loss occurs during staging which clears the UARP stack state, an accessory can optionally support
accessory-driven resume. In such a scenario, the `UARPRequestPayloadByIndex()` and `UARPSetPayloadDataOffset()` APIs
may be used by the accessory to jump to the desired offset based on the portion of the asset retained in the staging
area. This may be beneficial for accessories with large updates or those utilizing a transport with limited throughput,
resulting in lengthy staging times. Note that the accessory implementation is responsible for maintaining the staging
progress, performing asset comparison, and any validation of the partial asset to support accessory-driven resume. The
sample implementation uses the key value store for persisting payload progress. This allows for resuming after reset or
power loss. However, depending on the data chunk size being used and image size, this can result in a substantial
number of flash write cycles to maintain the progress state. Alternatively, non-initialized RAM could be used to
support resume across just resets, a resume offset could be determined by examining the staging area contents, or the
storage of progress to persistent memory could be performed less frequently (e.g. every N payload data callbacks) with
appropriate resume cleanup to lessen the overhead and flash wear. How, and even whether, accessory-driven resume should
be supported is left up to the accessory vendor based on system design.

#### Competing offers
After an asset has been accepted, it is possible for a competing asset to be offered to the accessory. This may be done
either while actively staging the initial asset or after the initial asset has been staged but not applied. If the
accessory wishes to accept the new asset, it must first explicitly abandon the previously accepted asset. This is done
by calling `UARPRequestAssetStagingStateChange()` with `kUARPAssetStagingStateChangeRequest_Abandon`. Note that an
offer made to the accessory layer will have a version which has passed the initial version checks against the active
firmware and any fully staged version. In addition, if actively staging an asset, the offered version will also be
greater than that which is currently being staged.

#### Minimum firmware version
SuperBinary assets may be packaged such that any required minimum running version is available alongside the desired
update version. In such a scenario, the accessory can pull the payload(s) corresponding to the minimum version and
indicate staging is complete. The "Apply Needed" staging not ready reason needs to be set by the accessory to indicate
to the controller that the currently staged version needs to be applied in order to update to the firmware version
equivalent to that of the asset offer. Upon applying the minimum version and receiving an offer for the initial
SuperBinary asset again, the payload(s) corresponding to the final version can then be pulled. Note that the accessory
firmware would need to have knowledge of minimum firmware versions and appropriate handling baked into the running
version upon receiving the asset offer. More information can be found in *Section 3 - SuperBinary* of the UARP
development guide.

#### Transport Optimization
The UARP protocol and HDS add overhead to the raw asset data that is being delivered to the accessory.
- Each HDS frame includes a 4 byte header and 38 bytes of overhead associated with the HDS stream protocol.
- Each UARP data response message header is 18 bytes.

For IP accessories, an authentication tag of 16 bytes is appended to each HDS frame in accordance with HDS over TCP
frame format. Therefore, each chunk of staging data has an associated 76 bytes of overhead. The primary factor which
can be used to improve staging performance is the data chunk size, `kUARPDataChunkSize`, at a memory cost to support
larger buffers. The default data chunk size for IP accessories is 8 KB.

For BLE accessories, there is additional overhead associated with the BLE HAP PDU and HDS HAP transport:
- The HAP PDU header and PDU body overhead is 10 bytes. If the HAP PDU is fragmented across multiple BLE packets, the
  HAP PDU Header is 2 bytes for subsequent BLE packets.
- Each BLE packet includes a 16 byte HAP PDU Auth Tag.
- HDS data sent over the HAP transport is packed in TLV8 chunks, adding 2 bytes of overhead for every 255 bytes of HDS
  data. The HAP transport also adds 3 bytes of overhead per write to the transport characteristic to specify the
  session identifier associated with the data stream.

The data chunk size is again the primary variable for improving staging throughput. In addition to decreasing the
overhead per byte of staging data, larger data chunks decrease the number of write-responses the controller must
perform on the HAP transport characteristic.

Another factor to take into account is the BLE GATT MTU. The larger the MTU, the less HAP PDU overhead and the fewer
write-responses required for a fixed staging data size. For example, consider the default data chunk size for BLE
accessories of 512 bytes with a BLE MTU of 512 bytes as well.

```sh
BLE Packet 1:

HAP PDU header and body overhead: 10 bytes
    HAP transport TLV (payload): 2 bytes
        HDS header: 46 bytes
            UARP data response header: 18 bytes
                UARP data response payload: 191 bytes
    HAP transport TLV (payload): 2 bytes
                UARP data response payload: 227 bytes
HAP PDU auth tag: 16 bytes

BLE Packet 2:

HAP PDU header: 2 bytes
                UARP data response payload: 28 bytes
    HAP transport TLV (payload): 2 bytes
                UARP data response payload: 66 bytes
    HAP transport TLV (session id): 3 bytes
HAP PDU auth tag: 16 bytes
```

In this case, the first BLE PDU fragment carries 418 bytes of staging data while the remaining 94 bytes correspond to
message overhead. The second BLE PDU fragment carries 94 bytes of staging data with 23 bytes of overhead and the
remaining 395 bytes of the MTU unused.

Note that the BLE GATT MTU is negotiated with iOS and therefore may not be statically optimized, but in general support
for a larger MTU on the accessory side improves the chances of lessening the overhead associated with staging an update
to the device.
