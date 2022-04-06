Accessory Diagnostics Service
=============================

## Overview
This service enables the retrieval of diagnostic data from the accessory. An accessory is responsible to capture and
upload the logs to a HomeKit controller. The zipped archive or text file will be displayed on a share sheet on an iOS
controller.
If manufacturer diagnostics is enabled for the accessory, the user will have an option to upload the logs to an
accessory vendor. The vendor URL to forward the logs is determined at the time of certification.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK.
Once all the prerequisites are installed for your platform, run the following to compile ADK with the Accessory
Diagnostics feature:

``` tabs::

   .. group-tab:: macOS

    Enable diagnostics snapshot format - *zip* (default)

    .. code-block:: bash

     make TARGET=Darwin HAP_DIAGNOSTICS_SERVICE=1

    Enable diagnostics snapshot format - *text*

    .. code-block:: bash

     make TARGET=Darwin HAP_DIAGNOSTICS_SERVICE=1 USE_DIAGNOSTICS_TEXT_FORMAT=1

    Enable diagnostics snapshot type - *ADK* (default)

    .. code-block:: bash

     make TARGET=Darwin HAP_DIAGNOSTICS_SERVICE=1

    Enable diagnostics snapshot type - *Manufacturer*

    .. code-block:: bash

     make TARGET=Darwin HAP_DIAGNOSTICS_SERVICE=1 HAP_DIAGNOSTICS_MANUFACTURER=1

    Enable diagnostics service for BLE:

    .. code-block:: bash

     make TARGET=Darwin PROTOCOLS=BLE HAP_DIAGNOSTICS_SERVICE=1 HAP_HDS_TRANSPORT_OVER_HAP=1

    Enable diagnostics service for Thread:

    .. code-block:: bash

     make TARGET=Darwin PROTOCOLS=THREAD HAP_DIAGNOSTICS_SERVICE=1 HAP_HDS_TRANSPORT_OVER_HAP=1

   .. group-tab:: Linux

    Enable diagnostics snapshot format - *zip* (default)

    .. code-block:: bash

     make TARGET=Linux HAP_DIAGNOSTICS_SERVICE=1

    Enable diagnostics snapshot format - *text*

    .. code-block:: bash

     make TARGET=Linux HAP_DIAGNOSTICS_SERVICE=1 USE_DIAGNOSTICS_TEXT_FORMAT=1

    Enable diagnostics snapshot type - *ADK* (default)

    .. code-block:: bash

     make TARGET=Linux HAP_DIAGNOSTICS_SERVICE=1

    Enable diagnostics snapshot type - *Manufacturer*

    .. code-block:: bash

     make TARGET=Linux HAP_DIAGNOSTICS_SERVICE=1 HAP_DIAGNOSTICS_MANUFACTURER=1

   .. group-tab:: Raspberry Pi

    Enable diagnostics snapshot format - *zip* (default)

    .. code-block:: bash

     make TARGET=Raspi HAP_DIAGNOSTICS_SERVICE=1

    Enable diagnostics snapshot format - *text*

    .. code-block:: bash

     make TARGET=Raspi HAP_DIAGNOSTICS_SERVICE=1 USE_DIAGNOSTICS_TEXT_FORMAT=1

    Enable diagnostics snapshot type - *ADK* (default)

    .. code-block:: bash

     make TARGET=Raspi HAP_DIAGNOSTICS_SERVICE=1

    Enable diagnostics snapshot type - *Manufacturer*

    .. code-block:: bash

     make TARGET=Raspi HAP_DIAGNOSTICS_SERVICE=1 HAP_DIAGNOSTICS_MANUFACTURER=1

    Enable diagnostics service for BLE:

    .. code-block:: bash

      make TARGET=Raspi PROTOCOLS=BLE HAP_DIAGNOSTICS_SERVICE=1 HAP_HDS_TRANSPORT_OVER_HAP=1

   .. group-tab:: Nordic nRF52

    Enable diagnostics snapshot format - *text*

    .. code-block:: bash

     make TARGET=nRF52 HAP_DIAGNOSTICS_SERVICE=1 HAP_HDS_TRANSPORT_OVER_HAP=1 USE_DIAGNOSTICS_TEXT_FORMAT=1

    Enable diagnostics snapshot type - *ADK* (default)

    .. code-block:: bash

     make TARGET=nRF52 HAP_DIAGNOSTICS_SERVICE=1 HAP_HDS_TRANSPORT_OVER_HAP=1 USE_DIAGNOSTICS_TEXT_FORMAT=1

    Enable diagnostics snapshot type - *Manufacturer*

    .. code-block:: bash

     make TARGET=nRF52 HAP_DIAGNOSTICS_SERVICE=1 HAP_HDS_TRANSPORT_OVER_HAP=1 USE_DIAGNOSTICS_TEXT_FORMAT=1 HAP_DIAGNOSTICS_MANUFACTURER=1

    For Thread:

    .. code-block:: bash

     make TARGET=nRF52 PROTOCOLS=THREAD HAP_DIAGNOSTICS_SERVICE=1 HAP_HDS_TRANSPORT_OVER_HAP=1 USE_DIAGNOSTICS_TEXT_FORMAT=1

```

### Run
Please follow instructions at [Running an ADK Application](getting_started.html#step-5-running-an-adk-application) to
run an ADK sample application.

## Implementation Details

### Key Requirements
- Vendors would need to comply with privacy guidelines for the logs to be included in Accessory Diagnostics.
- Accessory diagnostics feature requires that the platform supports a file system to which the diagnostics file can be
written and persisted.
- Accessory platform should support an archiving utility to be able to archive the logs before they are sent over to an
iOS device.
- Size of zip file or text file may not exceed 5 MB.
- Multiple simultaneous requests should not be allowed.
- Only admin controllers can request for diagnostics logs.

### Additional Information
- Bytes are uploaded in payload data chunks of 400 KB for IP transport and 7 KB for Bluetooth and Thread transports.
  Including URL parameters the packet size is configured to be at most *500 KB*. See *kHAPDiagnostics_NumChunkBytes*.
  Specification mentions any value between *1 KB* and *900 KB* may be chosen. This parameter may need to be tweaked
  based on transport type. This is not configurable by the accessory application.
- HAP Log Capture:
  - Diagnostics only provides ability to capture HAP logs to file. Other logs like pcap, system, etc. can still be sent
    using Diagnostics but its capture is outside the scope of the sample implementation provided by ADK.
  - Captured HAP logs are persistent across crashes or app reboots.
  - Uses a rotation scheme of 10 log files to keep total HAP Log size below a configurable accessory specified value.

### HAP Implementation
- *HAP+API.h*
  - Service and characteristics information
  - Diagnostics configuration structure
    - Diagnostics snapshot format
    - Diagnostics snapshot type
    - Diagnostics snapshot options
    - Diagnostics supported mode
    - Diagnostics context
- *HAP/HAPDataStreamProtocols+DataSend.h* and *HAP/HAPDataStreamProtocols+DataSend.h*
  - `HAPDataSendDataStreamProtocolSendData` dataSequenceNumber for packets and URL parameters
  - Added new optional reason "DiagnosticsData"
- *HAP/HAPDiagnostics.h* and *HAP/HAPDiagnostics.c*
  - `HAPDiagnosticsContext` structure
    - Data Send stream context
    - Data Send scratch bytes
    - Other internal structures
  - *HAPDiagnosticsHandleDataSendStreamAvailable*
    - HDS callback function
    - Sets up dataSend stream & context
    - Calls PAL API `HAPPlatformDiagnosticsPrepareData` to collect logs and create a zip or text file in a separate
      thread/process
    - Creates a timer to monitor data prepare operation
    - Rejects simultaneous requests
  - Callback APIs for PAL to report status to HAP
    - `HAPDiagnosticsDataReady` called by PAL to report diagnostics file is ready
    - `HAPDiagnosticsDataCancel` called by PAL to report cancellation of data prepare operation
  - *HandleDataSendStreamClose*
    - HDS callback function
    - Clears Data Send stream context state and other internal flags
  - *UploadDiagnosticsData*
    - Calls into PAL to get the bytes to be uploaded
    - Creates Data send stream protocol packets and sends data using HAPDataSendDataStreamProtocolSendData
  - *HAPDiagnosticsSetupLogCaptureToFile* and *HAPDiagnosticsStopLogCaptureToFile*
    - Sets up & stops capture of HAP log file
    - Calls into PAL APIs

### PAL Implementation
- *PAL/HAPPlatformDiagnostics.h* and *PAL/POSIX/HAPPlatformDiagnostics.c*
  - `HAPPlatformDiagnosticsInitialize` initializes diagnostics, creates diagnostics folders and starts capture of PAL
      specific diagnostics
  - `HAPPlatformDiagnosticsDeinitialize` stops capture of PAL specific diagnostics and de-initializes diagnostics
  - `HAPPlatformDiagnosticsStartLogCaptureToFile` starts capturing logs to Diagnostics log file.
  - `HAPPlatformDiagnosticsWriteToFile` function is called when HAP log API is called. When diagnostics logging is
      enabled, HAP logs are written both to stderr and to file.
  - `HAPPlatformDiagnosticsWriteToLogBuffer` function is called when HAP logs are to be written to a buffer instead
      of a file.
  - `HAPPlatformDiagnosticsGetBytesToUpload` reads chunk bytes from diagnostics file for HAP to upload.
  - `HAPPlatformDiagnosticsPrepareData` called by HAP to start the data collection operation asynchronously.
  - `HAPPlatformDiagnosticsDataTransferComplete` called by HAP to inform PAL that the data transfer is complete. PAL
      may free up its resources.
  - `HAPPlatformDiagnosticsAbort` called by HAP to abort log collection as the data stream has been closed.
  - `HAPPlatformDiagnosticsWriteToCrashLog` called by App if a segmentation fault is detected.
  - `HAPPlatformDiagnosticsClearCrashLog` called by App to clear the crash log.
  - `HAPPlatformDiagnosticsWriteToConfigLog` called by HAP to log accessory configuration information.
- *PAL/HAPPlatformDiagnosticsLog.h* and *PAL/POSIX/HAPPlatformDiagnosticsLog.c*
  - `DiagnosticsLog` context structure.
  - Handles rotation of log files to restrict total HAP log size.
