Apple TV Remote
===============

## Overview
The Apple TV Remotes profile allows a HomeKit accessory to control a target, which is an Apple TV. This document
describes the Apple TV Remote features in ADK. Please refer to *ADK/Applications/Remote* source for an example
implementation. Please also refer to section *Remotes for Apple TV* in HomeKit Accessory Protocol Specification.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK. Once all
the prerequisites are installed for your platform, run the following to compile ADK with Apple TV Remote features:

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin APPS=Remote

      To enable Siri functionality for the Remote:

      .. code-block:: bash

       make TARGET=Darwin USE_SIRI_REMOTE=1 APPS=Remote

   .. group-tab:: Linux

      .. code-block:: bash

       make TARGET=Linux APPS=Remote

      To enable Siri functionality for the Remote:

      .. code-block:: bash

       make TARGET=Linux USE_SIRI_REMOTE=1 APPS=Remote

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       make TARGET=Raspi APPS=Remote

      To enable Siri functionality for the Remote:

      .. code-block:: bash

       make TARGET=Raspi USE_SIRI_REMOTE=1 APPS=Remote

   .. group-tab:: Nordic nRF52

      .. Warning::
          This feature is currently not supported on this platform.

```

### Run
Please follow instructions at [Running an ADK Application](getting_started.html#step-5-running-an-adk-application) to
run the Remote sample application.

## Implementation Details

As this profile needs relatively complex logic that is the same for all products, that common logic has been factored
out into the helper files *Remote.h/.c*. In order to simplify migration to future ADK versions, it is recommended that
a licensee not modify these helper files - so that they can easily be replaced by newer versions.

The Apple TV Remotes profile uses the HomeKit Data Stream (HDS) protocol for connections to a HomeKit hub (Apple TV).
A Remote service requires at least *8 KB* of RAM for HDS with the recommendation of *16 KB*. In addition, about *160 KB*
of RAM is needed for audio buffers and other data structures used in the Remote accessory logic and in the
microphone PAL module (e.g., audio buffers).

The maximum number of supported targets can be configured. In the Remote sample, every target gets its own key-value
store domain, defined in *Remote.c*:

```c
/** * Key value store domain for the first target of the remote. */
#define kAppKeyValueStoreDomain_TargetBegin    ((HAPPlatformKeyValueStoreDomain) 0x01)

/** * Key value store domain for the last target of the remote. */
#define kAppKeyValueStoreDomain_TargetEnd      ((HAPPlatformKeyValueStoreDomain) 0x14)
```

In such a domain, the configuration of the target (e.g., Apple TV), is stored. From this information, the maximum number
of targets is computed as `kRemote_MaxTargets`. The configuration of the Remote itself is stored in another domain
`(kAppKeyValueStoreDomain_Configuration)`.

## Raspberry Pi as Remote Example

A Raspberry Pi may also be used to demonstrate a remote control accessory. A Remote accessory requires a HomeKit hub,
i.e., an Apple TV. Here, an Apple TV is assumed as the hub, an iPhone as the iOS controller, and a Raspberry Pi as the
Remote accessory. The Remote sample application allows controlling an Apple TV with configurable buttons. The Remote
sample additionally contains support for Siri.

### Device Setup
#### iPhone Setup
- Make sure that the iPhone runs iOS 12 or later.
- Set the region to the United States and the language to English.
- Set up a HomeKit home.

#### Apple TV Setup
- Make sure that the Apple TV runs tvOS 12 or later.
- Assign the Apple TV to the same Wi-Fi network as the iPhone, to the same Apple ID (iCloud ID) as the iPhone, and to
the same home as the iPhone.
- Set the region to the United States and the language to English.
- Enable AirPlay.
- Turn on Home Sharing.
- Make sure that iCloud and Home Sharing are connected to the same iCloud account.

``` Important::
    The remote supports connecting to multiple Apple TVs on the same network.
```

### Usage
The Remote application captures keyboard input from the console to emulate key presses. Key up and key down events are
sent separately to emulate physical button events and to allow for easier testing. The following key configuration is
configured in the remote:

Action            | Down | Up
------------------| ---- | ----
Siri              | 1    | 2
Menu              | q    | w
Play / Pause      | e    | r
TV / Home         | t    | y
Select            | u    | i
Arrow Up          | a    | s
Arrow Right       | d    | f
Arrow Left        | g    | h
Volume Up         | j    | k
Volume Down       | l    | ;
Power             | c    | v
Generic           | b    | n

Before key presses can be sent, the remote needs to be paired with a controller that has at least one Apple TV
configured. If multiple Apple TVs are configured, the *m* key can be used to switch between the configured Apple TVs.
The *0* key is used to emulate a remote that controls a non-HomeKit entity.

Action                       | Key
-----------------------------| ----
Toggle Identifier / Apple TV | m
Set target to non HomeKit    | 0

``` Note::
    These key configurations also work for the remote bridge. Remote 1 will react to the key presses that are discussed
    above. Remote 2 reacts to the same key presses with the additional Shift  key pressed (assumption: US/International
    keyboard layout).
```

### Background Information
Apple TVs are identified by an integer number. The Active Identifier denotes the currently active identifier, e.g.,
Apple TV. Each Apple TV will register itself with an identifier and the associated key configuration.

If the active identifier is set to *0*, the device indicates that it is either controlling a non-HomeKit entity or that
no Apple TV has been configured yet. In this case the following log message will be shown:

```sh
2018-10-17'T'18:12:34'Z'    Debug    [com.apple.mfi.HomeKit.Remote:Remote] RemoteRaiseButtonEvent: No active identifier is set for the remote registered with remote 0x86388.
```

``` Note::
    If this message occurs even tough the Remote has been paired and an Apple TV is configured, then verify the Apple
    TV/iPhone/Remote configuration discussed earlier.
```
