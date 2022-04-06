Accessory Metrics Service
=========================

## Overview
This feature enables an accessory to report metrics to an iOS device. Accessory metrics are collected and uploaded to
Apple Analytics system. This document describes the Accessory Metrics feature provided by the ADK and how to integrate
it into a HomeKit accessory.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK.
Once all the prerequisites are installed for your platform, run the following to compile ADK with the Accessory Metrics
feature:

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin USE_ACCESSORY_METRICS=1

   .. group-tab:: Linux

      .. code-block:: bash

       make TARGET=Linux USE_ACCESSORY_METRICS=1

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       make TARGET=Raspi USE_ACCESSORY_METRICS=1

   .. group-tab:: Nordic nRF52

      .. Warning::
          This feature is currently not supported on this platform.

```

### Run
Please follow instructions at [Running an ADK Application](getting_started.html#step-5-running-an-adk-application) to
run an ADK sample application.

## Implementation Details

### Key Requirements
- The accessory may choose to only report metric events defined in *HAPMetricEvent.h*.
- A batch of metric data packets are sent every *60 seconds* to a connected controller via an open HDS session.
- Multiple simultaneous HDS metrics sessions is not allowed.
- Only admin controllers can request for metrics data.

### Additional Information
- Metrics events are sent as binary data packets over HDS. Each data packet is a metric event of type TLV8.
- Each supported metric event is described in *HAPMetricEvent.h*. Unsupported metric events must not be sent.
- The maximum queue size to store metric events is defined by `kHAPMetrics_MaxStoredEvents`. The default value is 50 events.
  This value can be overridden at compile time using the option HAP_METRICS_MAX_STORED_EVENTS. Every
  `kHAPMetrics_SendInterval` seconds the data in this queue is sent over HDS and flushed.
- The TLV8 packet data for each metric event should not exceed `kHAPMetrics_MaxEventBytes` bytes.
- The accessory will provide a list of supported metrics to an admin controller using the Supported Metrics
  characteristic. The controller may choose to override the default reporting status of the metric.
- The controller may choose to enable or disable all metrics collection via the Active characteristic .

### HAP Implementation
- *HAPMetricsEvent.h* and *HAPMetricEvent.c*
  - Metric event definitions.
  - Metric event setup APIs.
    - `HAPMetricsEventInitialize` - Initialize a single metric event.
    - `HAPMetricsEventSetFieldValue` - Set values for members of the metric event.
    - `HAPMetricsEventEncodeTLV` - Encode the metric event to TLV.
- *HAPMetrics.h* and *HAPMetrics.c*
  - Manages metrics event queue.
  - Sends data over HDS to controller.
  - `HAPMetricsSubmitMetricEvent` - Submit metric event to the metric queue.
  - `HAPMetricsDeleteMetricsEvents` - Deletes all previously queued metric events of a specific metric ID.
  - `HAPMetricsDeleteAllMetricEvents` - Deletes all previously queued metric events.

### App Implementation
- *MetricsHelper.h* and *MetricsHelper.c*
  - Calls HAP APIs to initialize and de-initialize metrics framework.
  - Implements read and write handlers for the Accessory Metrics Characteristics.
- *MetricsServiceDB.h* and *MetricsServiceDB.c*
  - Accessory Metrics service and Characteristics configuration.
  - Service and Characteristics IIDs.
