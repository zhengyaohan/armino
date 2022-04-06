Adaptive Lighting Service
=========================

## Overview
This document describes the Adaptive Lighting feature provided by the ADK and how to integrate it into a HomeKit
accessory. This feature enables HomeKit-based accessories to change Brightness and Color Temperature based on a given
schedule. Please refer to *Light Shift Requirements* section of HomeKit Accessory Protocol Specification for more
details.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK. Once all the
prerequisites are installed for your platform, run the following to compile ADK with adaptive lighting feature:

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin HAP_ADAPTIVE_LIGHT=1 APPS=Lightbulb

   .. group-tab:: Linux

      .. code-block:: bash

       make TARGET=Linux HAP_ADAPTIVE_LIGHT=1 APPS=Lightbulb

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       make TARGET=Raspi HAP_ADAPTIVE_LIGHT=1 APPS=Lightbulb

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       make TARGET=nRF52 HAP_ADAPTIVE_LIGHT=1 APPS=Lightbulb

```

### Run
Please follow instructions at [Getting Started](getting_started.md) page to run Lightbulb sample application.

## Implementation Details
HomeKit controller configures Characteristic Transition Schedule after querying Supported Transitions Settings
from the accessory. If proposed Transition Configuration is valid, the accessory will accept it and start executing
immediately. The accessory is expected to update respective characteristic value based on schedule and notify
controllers once transition schedule is complete.

### Assumptions
- Number of Supported Transition Points: 52
  - This is the number of transition points supported across all transitions. It is not the number of transition points
    supported per transition.
  - The number of transition points required may increase in the future, and vendors shall adjust the value based on
    features supported by accessories.
- Number of Supported Transitions: 2
- Size of Controller Context: 255 bytes
- Data type for storing characteristic value: int32_t
- Data type for storing HAP Instance ID: uint64_t
- Vendors can reduce memory usage:
  - If they support transition for one characteristic only.
  - If they use fewer bytes to represent HAP instance ID in their application.

### Terminology
- *Transition*: A curve describing change in characteristic value over time.
- *Transition point*: A point on the transition curve describing change relative to the previous point.
- *Linear Transition*: Transition where value of the given characteristic is independent of any other characteristics.
- *Linear Derived Transition*: Transition where value of the given characteristic depends on value of some other
characteristic.
- *Loop*: End Behavior where Transition repeats itself after last transition point is complete.

### Additional Information
Please follow below steps to use this feature in your accessory:
- Include *AdaptiveLight.h* in your application.
- Advertise appropriate range for Brightness/Color Temperature characteristic.
- Define permanent storage for Adaptive Light feature, ie a static instance of type `AdaptiveLightTransitionStorage`.
- Implement callback methods `handleCharacteristicValueUpdate`, `handleTransitionExpiry`,
`handleCharacteristicValueRequest`.
- Invoke `InitializeAdaptiveLight()` with appropriate parameters during application initialization.
- Invoke `RemoveTransition()` during write handler of a characteristic if it is in middle of a transition.

### App Implementation
- *Applications/LightBulb/AdaptiveLight.h*:
  - `struct AdaptiveLightTransitionStorage`: The persistent storage for Transition configuration and context.
  - `struct AdaptiveLightTransition`: A common structure used to represent Linear and Linear Derived Transitions.
  - `struct AdaptiveLightTransitionPoint`: A common structure used to represent Linear and Linear Derived Transition
    points.
  - `struct AdaptiveLightCallbacks`: A collection of callback functions to be implemented by the Application.
  - `struct AdaptiveLightRuntimeContext`: Runtime context required to remember write response type.
  - `InitializeAdaptiveLightParameters()`: Function to configure storage, callback function, and supported transition
types.

    ```c
    /**
     * Initialize Adaptive Light Storage
     *
     * @param      adaptiveLight            Pointer to Persistent Storage
     * @param      supportedTransitions     Supported Transitions
     * @param      numSupportedTransitions  Number of Supported Transitions
     * @param      callback                 Callbacks to be invoked for requests and notifications
     *
     * @return kHAPError_None           If successful.
     * @return kHAPError_OutOfResources If number of requested transitions is more than supported.
     */
    HAP_RESULT_USE_CHECK
    HAPError InitializeAdaptiveLightParameters(
            HAPPlatformKeyValueStoreRef keyValueStore,
            AdaptiveLightTransitionStorage* adaptiveLight,
            AdaptiveLightSupportedTransition* supportedTransitions,
            size_t numSupportedTransitions,
            AdaptiveLightCallbacks* callbacks);
    ```

  - `HAPHandleSupportedTransitionConfigurationRead()`: Read handler for supported Transition Configuration
     characteristic.
  - `HAPHandleTransitionControlRead()`/`HAPHandleTransitionControlWrite()`: Read/Write handler for Transition Control
    characteristic.
  - Callback methods to be implemented by the Application:

    ```c
    /**
     * Callback function invoked when characteristic value changes during transition.
     *
     * @param characteristicID   HAP Instance ID of the characteristic.
     * @param value              New value
     * @param sendNotification   Boolean to indicate if app should notify controller of value change
     */
    void (*handleCharacteristicValueUpdate)(uint64_t characteristicID, int32_t value, bool sendNotification);
    /**
     * Callback function invoked when a transition gets over.
     *
     * @param characteristicID   HAP Instance ID of the characteristic.
     */

    void (*handleTransitionExpiry)(uint64_t characteristicID);
    /**
     * Callback function invoked to obtain current value of given characteristic.
     *
     * @param characteristicID   HAP Instance ID of the characteristic.
     * @param value              Output value
     */
    void (*handleCharacteristicValueRequest)(uint64_t characteristicID, int32_t* value);
    ```

- *Applications/Common/Helper/AdaptiveLight.c*:
This file contains source code for characteristic read/write handlers, transition value calculations, and characteristic
transitions.
