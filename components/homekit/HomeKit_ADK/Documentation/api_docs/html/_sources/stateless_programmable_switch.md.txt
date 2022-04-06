Stateless Programmable Switch
=============================

## Overview
This document provides an overview of how to build a Stateless Programmable Switch. Please refer to
*Stateless Programmable Switch* section of HomeKit Accessory Protocol Specification for more details.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK.
Once all the prerequisites are installed for your platform, run the following to compile the
*StatelessProgrammableSwitch* sample application:

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin APPS=StatelessProgrammableSwitch

   .. group-tab:: Linux

      .. code-block:: bash

       make TARGET=Linux APPS=StatelessProgrammableSwitch

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       make TARGET=Raspi APPS=StatelessProgrammableSwitch

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       make TARGET=nRF52 APPS=StatelessProgrammableSwitch

```

### Run
Please follow instructions at [Running an ADK Application](getting_started.html#step-5-running-an-adk-application) to
run an ADK sample application.

## Implementation Details
There are three Stateless Programmable Switch events:
- Single Press
- Double Press
- Long Press

When defining the Stateless Programmable Switch event characteristic, make sure that the readable property is set to
*true*:

```c
static const HAPUInt8Characteristic
    programmableSwitchEventCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_ProgrammableSwitchEvent,
    .characteristicType = &kHAPCharacteristicType_ProgrammableSwitchEvent,
    .debugDescription =
        kHAPCharacteristicDebugDescription_ProgrammableSwitchEvent,
    .manufacturerDescription = …,
    .properties = {
        .readable = true,
        .writable = false,
        …
```

If the readable property is not set to true, iOS may not be able to pair with the accessory. On *IP* accessories,
`handleRead` for this characteristic is only invoked in response to RaiseEvent. On *IP* accessories, null is returned
for Paired Reads and handleRead is not called. The implementation of this function should return the last event
triggered. This ensures that the code works as specified on both *IP* and *BLE*.
