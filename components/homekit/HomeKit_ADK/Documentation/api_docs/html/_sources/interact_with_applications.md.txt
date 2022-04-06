Interacting with ADK applications
=================================

## Overview
There are several ways to control the behavior of an accessory for debug purposes.
 - Button presses on platforms that support physical button such as Nordic `nRF52840`
    - Buttons `1` through `4`
 - Signals on platforms that support UNIX signals such as `Raspberry Pi` and `macOS`
    - `SIGUSR1`
    - `SIGUSR2`
    - `SIGTERM`
    - `SIGQUIT`
 - Debug commands via Debug Command Service
 - Debug commands via input file
 - Application command line arguments

## Buttons and Unix Signals
To send a UNIX signal to a running application:

``` tabs::

    .. group-tab:: macOS

        - killall -<signal number> <application name>

    .. group-tab:: Raspberry Pi

        - sudo kill -<signal number> <PID of application>

```

For example, to send `SIGUSR1` UNIX signal to the Lightbulb sample application:

``` tabs::

    .. group-tab:: macOS

        - In Terminal window 1, launch the sample ADK application ``./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/Lightbulb.OpenSSL``
        - In Terminal window 2, run ``killall -SIGUSR1 Lightbulb.OpenSSL``

    .. group-tab:: Raspberry Pi

        - In Terminal window 1, SSH into the Raspberry Pi and launch the sample ADK application ``sudo ./Lightbulb.OpenSSL``
        - In Terminal window 2, run ``sudo kill -SIGUSR1 `pidof Lightbulb.OpenSSL```

```

### Button 1 / SIGUSR1
All applications on receiving this button press or signal simulate clearing of the accessory pairing to a controller.

### Button 2 / SIGUSR2
All applications on receiving this button press or signal simulate factory reset.

### Button 3 / SIGTERM
All applications on receiving this button press or signal simulate triggering pairing mode. For accessories with NFC programmable tag (compiled with `USE_NFC=1`), this enters pairing mode. For non-NFC accessories, this starts advertisement.

### Button 4 / SIGQUIT
Behavior of each application may be customized.

## Debug Commands

Debug commands can be issued via the Debug Command service or an input file.

### Debug Command Service
The ADK exposes a debug service which allows sample applications to provide an interface for debug purposes. The service uses UUID 000040DE-E8F4-40DF-A3D2-B4A3A182CFC6 and includes two characteristics: Short Debug Command Line and Debug Command Line. The former accepts string-formatted data while the latter uses a raw hex data format. Both characteristics require the timed write permission. Using the HAT tool, user can send debug commands to an ADK application.

For example, to issue `toggleState` debug command to the Lightbulb sample application:

``` tabs::

    .. tab:: Short Debug Command

        - In Terminal window, launch the sample ADK application ``./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/Lightbulb.OpenSSL``
        - In HAT tool, pair with Lightbulb accessory and Discover
        - Locate characteristic with UUID 000040DF-E8F4-40DF-A3D2-B4A3A182CFC6
        - Perform a Timed Write with ``toggleState``

    .. tab:: Debug Command

        - In Terminal window, launch the sample ADK application ``./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/Lightbulb.OpenSSL``
        - In HAT tool, pair with Lightbulb accessory and Discover
        - Locate characteristic with UUID 000040A3-E8F4-40DF-A3D2-B4A3A182CFC6
        - Perform a Timed Write with ``746F67676C655374617465``

```

### Input File
For platforms that support a file system, the ADK monitors and processes debug commands piped to an input file. The default input file is `.command.input` and located at the directory where the accessory is started. For debug builds, the name and location of this file can be changed using the `-c` or `--commandfile-path` when launching the application.

For example, to issue `toggleState` debug command to the Lightbulb application:

``` tabs::

    .. group-tab:: macOS

        - In Terminal window 1, launch the sample ADK application ``./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/Lightbulb.OpenSSL``
        - In Terminal window 2, ``echo toggleState > .command.input``

    .. group-tab:: Raspberry Pi

        - In Terminal window 1, SSH into the Raspberry Pi and launch the sample ADK application ``sudo ./Lightbulb.OpenSSL``
        - In Terminal window 2, ``echo toggleState > .command.input``

```

### Common Commands
#### Button Press
All ADK sample applications support the `button-press` debug command:

```sh
button-press [1|2|3|4]
    1 - Simulate clear accessory pairing on pressing button 1 or sending SIGUSR1
    2 - Simulate factory reset on pressing button 2 or sending SIGUSR2
    3 - Simulate trigger pairing mode on pressing button 3 or sending SIGTERM
    4 - Simulate custom behavior by the app on pressing button 4 or sending SIGQUIT
```

``` Note::
    Each app has its own custom behavior for button `3` and button `4` as documented above.
```

#### Firmware Update

Any ADK sample application built with the firmware update feature enabled (`HAP_FIRMWARE_UPDATE=1`) has the following additional commands available:

```sh
firmware-update
    setHAPState [-updateDuration  <decimal number (uint16_t)>]
                [-stagingNotReady <decimal number (uint32_t)>]
                [-updateNotReady  <decimal number (uint32_t)>]
                [-updateState     <decimal number  (uint8_t)>]
    setStagingNotReadyBit <bit index>
    clearStagingNotReadyBit <bit index>
    setUpdateNotReadyBit <bit index>
    clearUpdateNotReadyBit <bit index>
    setApplyDelay <decimal number (uint16_t)>
```

- `firmware-update setHAPState [options]` sets the state of the HAP firmware update profile based on the provided options. No validation is done on the option values. Any number of the available options may be included, with the requested state applied to the current state of the accessory HAP profile.
- `firmware-update setStagingNotReadyBit <bit index>` sets the corresponding bit in the HAP profile staging not ready reason. No validation is done on the provided bit index.
- `firmware-update clearStagingNotReadyBit <bit index>` clears the corresponding bit in the HAP profile staging not ready reason. No validation is done on the provided bit index.
- `firmware-update setUpdateNotReadyBit <bit index>` sets the corresponding bit in the HAP profile update not ready reason. No validation is done on the provided bit index.
- `firmware-update clearUpdateNotReadyBit <bit index>` clears the corresponding bit in the HAP profile update not ready reason. No validation is done on the provided bit index.
- `firmware-update setApplyDelay <delay>` sets the delay used to trigger the firmware version update when simulating an apply.

#### Asset Update

Any ADK sample application built with the asset update feature enabled (`USE_ASSET_UPDATE=1`) has the following additional commands available:

```sh
asset-update
    setNotReadyBit <bit index>
    clearNotReadyBit <bit index>
```

- `asset-update setNotReadyBit <bit index>` sets the corresponding bit in the HAP profile staging not ready reason. No validation is done on the provided bit index.
- `asset-update clearNotReadyBit <bit index>` clears the corresponding bit in the HAP profile staging not ready reason. No validation is done on the provided bit index.

#### Pairing Mode

All ADK sample applications have the following additional commands available:

```sh
pairing
    trigger
```

- `pairing trigger` triggers NFC programmable tag pairing mode if compiled with `USE_NFC=1`, otherwise starts advertisement

### Application Commands
#### Bridge

```sh
enumerateLightbulbsReachable
enumerateLightbulbsState
```

- `enumerateLightbulbsReachable` steps through the enumeration of the combined reachability states 00 -> 01 -> 10 -> 11 of the two bridged light bulbs
- `enumerateLightbulbsState` steps through the enumeration of the combined on states 00 -> 01 -> 10 -> 11 of the two bridged light bulbs

#### Fan

```sh
toggleActiveState
toggleTargetFanState
```

- `toggleActiveState` toggles the fan active state between active and inactive
- `toggleTargetFanState` toggles the target fan state between auto and manual

#### Faucet

```sh
setRemainingDuration <duration>
toggleFaucetStatusFault
toggleValveStatusFault
```

- `setRemainingDuration <duration>` sets the remaining duration
- `toggleFaucetStatusFault` toggles the status fault of the faucet service
- `toggleValveStatusFault` toggles the status fault of the valve service

#### Garage Door Opener

```sh
setCurrentDoorState
    open
    closed
    opening
    closing
    stopped
toggleObstructionDetected
```

- `setCurrentDoorState [open|closed|opening|closing|stopped]` changes the garage door current state
- `toggleObstructionDetected` toggles the garage door obstruction detected state

#### Humidifier/Dehumidifier

```sh
toggleActiveState
```

- `toggleActiveState` toggles the humidifier/dehumidifier active state between active and inactive

#### IP Camera

```sh
enterWacMode
```

- `enterWacMode` enters the WAC mode

#### IP Camera Bridge

```sh
enterWacMode
```

- `enterWacMode` enters the WAC mode

#### IP Camera Event Recorder

```sh
triggerMotion -m <media>
triggerMotionWithoutMedia
setNullMediaPath
changeResolution
disableCamera
enableCamera
setThirdPartyInactive
setThirdPartyActive
```

- `triggerMotion -m <media-file>` simulates motion trigger and, if on macOS, loads a media file to send over the data stream
- `triggerMotionWithoutMedia` simulates motion trigger without media if on macOS
- `setNullMediaPath` clears media file path
- `changeResolution` cycles through the `high`, `low` and `lowest` resolutions
- `disableCamera` simulates disabling of camera manually
- `enableCamera` simulates enabling of camera manually
- `setThirdPartyInactive` simulates third party inactive
- `setThirdPartyActive` simulates third party active

#### IP Camera Recorder Bridge

```sh
triggerMotion -m <media>
changeResolution
disableCamera
enableCamera
setThirdPartyInactive
setThirdPartyActive
```

- `triggerMotion -m <media-file>` simulates motion trigger and, if on macOS, loads a media file to send over the data stream
- `changeResolution` cycles through the `high`, `low` and `lowest` resolutions
- `disableCamera` simulates disabling of camera manually
- `enableCamera` simulates enabling of camera manually
- `setThirdPartyInactive` simulates third party inactive
- `setThirdPartyActive` simulates third party active

#### Lightbulb

```sh
toggleState
exitWacMode
```

- `toggleState` toggles the lightbulb state between on and off
- `exitWacMode` exits the WAC mode if compiled with `HAVE_IP` and `HAVE_WAC`

##### Adaptive Light

An ADK Lightbulb application built with the adaptive light feature enabled (`HAVE_ADAPTIVE_LIGHT`) has the following additional commands available:

```sh
setSupportedConfigForAdaptiveLight <value>
```

- `setSupportedConfigForAdaptiveLight <value>` sets supported transition types for Adaptive Light

Each byte of `<value>` specifies the different transition types:
- Bits 0-7 indicate transition type for Brightness
- Bits 8-15 indicate transition type for Color Temp

For each transition type, the 0th bit shows support for Linear Transition and the 1st bit for Linear Derived Transition.

Some example values:

| Value   | Color Temp Transition Type | Brightness Transition Type |
| :-----: | :------------------------: | :------------------------: |
| 0x0101  | Linear                     | Linear                     |
| 0x0201  | Linear Derived             | Linear                     |
| 0x0203  | Linear Derived             | Linear & Linear Derived    |
| 0x0100  | Linear                     | None                       |
| 0x0002  | None                       | Linear Derived             |

#### Lock

See the [Debug Command](lock_profile.html#debug-command) section of Lock Profile.

#### Outlet

```sh
toggleOutletState
```

- `toggleOutletState` toggles the outlet state between on and off

#### Remote

```sh
switchToNextTarget
```

- `switchToNextTarget` switches to the next available configured target

#### RemoteBridge

```sh
switchToNextBridge
```

- `switchToNextBridge` switches to the next bridge state

#### Security System

```sh
setSecuritySystemTargetState
    stayArm
    awayArm
    nightArm
    disarm
triggerAlarm
```

- `setSecuritySystemTargetState [stayArm|awayArm|nightArm|disarm]` changes the security system target state
- `triggerAlarm` simulates triggering of the security system alarm

#### Sensor

```sh
setAirQualityState
    unknown
    excellent
    good
    fair
    inferior
    poor
setCarbonDioxideDetectedState
    normal
    abnormal
setCarbonMonoxideDetectedState
    normal
    abnormal
setContactSensorState
    detected
    notDetected
setLeakDetectedState
    detected
    notDetected
toggleMotionDetectedState
setOccupancyDetectedState
    detected
    notDetected
setSmokeDetectedState
    detected
    notDetected
```

- `setAirQualityState [unknown|excellent|good|fair|inferior|poor]` changes the air quality state if compiled with `SENSOR_AIR_QUALITY`
- `setCarbonDioxideDetectedState [normal|abnormal]` changes the carbon dioxide detected state if compiled with `SENSOR_CARBON_DIOXIDE`
- `setCarbonMonoxideDetectedState [normal|abnormal]` changes the carbon monoxide detected state `SENSOR_CARBON_MONOXIDE`
- `setContactSensorState [detected|notDetected]` changes the contact sensor state if compiled with `SENSOR_CONTACT`
- `setLeakDetectedState [detected|notDetected]` changes the leak detected state if compiled with `SENSOR_LEAK`
- `toggleMotionDetectedState` toggles the motion detected state if compiled with `SENSOR_MOTION`
- `setOccupancyDetectedState [detected|notDetected]` changes the occupancy detected state if compiled with `SENSOR_OCCUPANCY`
- `setSmokeDetectedState [detected|notDetected]` changes the smoke detected state if compiled with `SENSOR_SMOKE`

#### Stateless Programmable Switch

```sh
buttonPress <index>
    single
    double
    long
```

- `buttonPress <index> [single|double|long]` simulates a button press for a button index

#### Television

```sh
setActive
setInactive
enterWacMode
```

- `setActive` simulates Television active
- `setInactive` simulates Television inactive
- `enterWacMode` enters the WAC mode

#### Video Doorbell

```sh
triggerMotion -m <media-file>
singleKeyPress
doubleKeyPress
longKeyPress
operatingStatus
changeResolution
disableCamera
enableCamera
setThirdPartyInactive
setThirdPartyActive
```

- `triggerMotion -m <media-file>` simulates motion trigger and, if on macOS, loads a media file to send over the data stream
- `singleKeyPress` simulates single key press
- `doubleKeyPress` simulates double key press
- `longKeyPress` simulates long key press
- `operatingStatus` simulates various Thermal Operating States if compiled with `HAVE_VIDEODOORBELL_OPERATING_STATE`
- `changeResolution` cycles through the `high`, `low` and `lowest` resolutions
- `disableCamera` simulates disabling of camera manually
- `enableCamera` simulates enabling of camera manually
- `setThirdPartyInactive` simulates third party inactive
- `setThirdPartyActive` simulates third party active

#### WiFi Router

```sh
ownership-proof-token
    generate
    enabled
    disabled
```

- `ownership-proof-token generate` generates a new ownership proof token for accessory setup
- `ownership-proof-token enabled` updates that ownership proof token is required for accessory setup
- `ownership-proof-token disabled` updates that ownership proof token is not required for accessory setup

```sh
kHAPCharacteristicDebugDescription_NetworkClientProfileControl
    list
    next-identifier
    add
    update
    remove
```

- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl list` lists the client's network profile
- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl next-identifier` gets the next client identifier to assign to a client
- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl add` adds a new client's network profile
- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl update` updates a client's network profile
- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl remove` removes a client's network profile

```sh
kHAPCharacteristicDebugDescription_NetworkClientStatusControl
    list
    connect
    disconnect
    add
```

- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl list` lists the network connections available to connect
- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl connect` connects a client to a network connection
- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl disconnect` disconnects a client from a network connection
- `kHAPCharacteristicDebugDescription_NetworkClientProfileControl add` adds a simulated network client status

```sh
kHAPCharacteristicDebugDescription_RouterStatus
    ready
    not-ready
```

- `kHAPCharacteristicDebugDescription_RouterStatus ready` sets the router status to ready
- `kHAPCharacteristicDebugDescription_RouterStatus not-ready` sets the router status to not-ready

```sh
kHAPCharacteristicDebugDescription_WANConfigurationList
    unconfigured
    other
    dhcp
    bridge
```

- `kHAPCharacteristicDebugDescription_WANConfigurationList unconfigured` sets the router WAN type to unconfigured
- `kHAPCharacteristicDebugDescription_WANConfigurationList other` sets the router WAN type to other
- `kHAPCharacteristicDebugDescription_WANConfigurationList dhcp` sets the router WAN type to dhcp
- `kHAPCharacteristicDebugDescription_WANConfigurationList bridge` sets the router WAN type to bridge mode

```sh
kHAPCharacteristicDebugDescription_WANStatusList
    update
```

- `kHAPCharacteristicDebugDescription_WANStatusList update` updates WAN status of a wireless network

```sh
kHAPCharacteristicDebugDescription_ManagedNetworkEnable
    enable
    disable
```

- `kHAPCharacteristicDebugDescription_ManagedNetworkEnable enable` enables the managed network
- `kHAPCharacteristicDebugDescription_ManagedNetworkEnable disable` disables the managed network

```sh
kHAPCharacteristicDebugDescription_NetworkAccessViolationControl
    add
```

- `kHAPCharacteristicDebugDescription_NetworkAccessViolationControl add` adds a simulated network access violation

```sh
kHAPCharacteristicDebugDescription_WiFiSatelliteStatus
    unknown
    connected
    not-connected
```

- `kHAPCharacteristicDebugDescription_WiFiSatelliteStatus unknown` sets the status of simulated Wi-Fi satellite accessory to unknown
- `kHAPCharacteristicDebugDescription_WiFiSatelliteStatus connected` sets the status of simulated Wi-Fi satellite accessory to connected
- `kHAPCharacteristicDebugDescription_WiFiSatelliteStatus not-connected` sets the status of simulated Wi-Fi satellite accessory to not-connected

#### Window Covering

```sh
toggleObstructionDetected
```

- `toggleObstructionDetected` toggles the window covering obstruction detected state

## Command Line Arguments
When using debug builds the following arguments can be passed when starting the application to customize
certain aspects of the accessory.

Common arguments:
```sh
-m PATH, --media-path PATH
           The path for Media files (macOS Only)
-a ACCESSORYNAME, --accessory-name ACCESSORYNAME
           The new name of the accessory.
-V, --version
            Prints the version of the APP.
-v FIRMWAREVERSION, --firmware-version FIRMWAREVERSION (ex: "1", "2.1.3")
           Override firmware version defined by application.
-r HARDWAREFINISH, --hardware-finish HARDWAREFINISH
           Override the hardware finish defined by application.
           e.g. --hardware-finish 0xECD6AA
-c COMMANDFILEPATH, --commandfile-path
           The path for the command line file. Default value is .command.input
           in the directory where the accessory is started.
           e.g. --commandfile-path /tmp/commands.txt
-k HOMEKITSTOREPATH, --homekitstorepath HOMEKITSTOREPATH
           The path for the Home Kit store
           e.g. --homekitstorepath "./.HomeKitStoreAbc"
-h, --help
           Print this usage.
```

For applications supporting HDS over HAP:

```sh
-H, --hds-over-hap
           Override Data Stream to use HAP transport (default is TCP)
```

For applications supporting Firmware Update:
```sh
-f, --fwup-persist-staging
           Use persistent storage for firmware update staging status
```

For example, to run a Lightbulb sample application with an alternate name and homekitstore:

```sh
sudo ./Lightbulb.OpenSSL --accessory-name "MyLightbulb" --homekitstorepath "~/.HomeKitStore2"
```
