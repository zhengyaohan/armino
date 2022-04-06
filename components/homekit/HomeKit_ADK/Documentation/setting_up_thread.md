Getting Started with Thread
===========================

## Overview
This document describes the support for Thread protocol in ADK and how to integrate it into a HomeKit accessory.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK. Once all the
prerequisites are installed for your platform, run the following to compile ADK with HAP over THREAD feature:

```sh
make TARGET=nRF52 PROTOCOLS=THREAD apps
```

### Run

```sh
./Tools/install.sh \
  -d nRF52 \
  -a Output/nRF52-arm-none-eabi/Debug/THREAD/Applications/Lightbulb.Oberon \
  -k \
  -t thread \
  -t ble
```

### Compile Flags
ADK supports the following Thread build flags:

* `USE_BLE`:     Set by default. May be set to 0 to build a Thread Only accessory. This is for testing purposes only,
Thread Only accessories are not supported by the HAP spec.
* `USE_STATIC_COMMISSIONING`: builds an accessory that is pre-commissioned with OpenThread default network credentials.
* `USE_THREAD_HOSTNAME`: Sets the host name to be used in mdns advertisements. If not set the default name
(Accessory name) will be used.
* `USE_MTD`: Builds a device as an MTD. Default is FTD.
* `DISABLE_THREAD_VIABILITY_CHECK`: Disable accessory check for network viability. ADK Thread accessories check for
Thread network availability every so often and fall back to BLE if the Thread network is not reachable. Additionally,
a minimal thread ADK accessory turns off the Thread network entirely to conserve power while it checks for Thread
network availability using an exponential back-off algorithm. This build flag should be turned off on production
accessories and is only recommended while going through OpenThread certification.

#### Examples:
- To build a Thread and BLE Full Thread Device (non-sleepy)

    ```sh
    make TARGET=nRF52 PROTOCOLS=THREAD apps
    ```

- To build a Thread only Full Thread Device which must be commissioned using Joiner

    ```sh
    make TARGET=nRF52 PROTOCOLS=THREAD USE_BLE=0 apps
    ```

- To build a Thread-only device that is statically commissioned

    ```sh
    make TARGET=nRF52 PROTOCOLS=THREAD USE_BLE=0 USE_STATIC_COMMISSIONING=1 apps
    ```

- To build a Thread and BLE full Thread Device with host name *Accessory 1*

    ```sh
    make TARGET=nRF52 PROTOCOLS=THREAD USE_THREAD_HOSTNAME="Accessory 1" apps
    ```

## Setting up your own thread network
In order to create our own Thread Network you will need:

- A Border Router
- At least one thread device to connect to the border router
- A computer to run HAT which will connect to the thread device over the Border Router.

### Setting up The Border Router
A border router is a device that bridges the Thread Network and a local IP/Wifi network. This section will describe how
to build a Border Router from a Raspberry Pi and an RCP.

- Raspberry Pi is responsible for running the *Thread Network Daemon* and *srp-mdns-proxy* which will provide the
Thread Network Accessory's mdns advertisements over the attached IP network.
- RCP is a device that implements the Thread Radio Control protocol over USB. The Daemon will use the RCP to
send / receive data over the Thread Network. This document describes how to build an NCP using a
[Nordic PCA10059](https://www.nordicsemi.com/Software-and-tools/Development-Kits/nRF52840-Dongle).

#### Setting up the Raspberry Pi

To set up the Raspberry Pi you must perform the following steps:

- Install Raspbian
- Build and launch the OpenThread daemon.
- Build and launch srp-mdns-proxy.

##### Installing Raspbian

- Download [Raspbian Buster Lite](https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2020-08-24/2020-08-20-raspios-buster-armhf-lite.zip)
- Install Raspbian on an SD Card with a tool such as *Balena Etcher*.
See [Raspbian Documentation](https://www.raspberrypi.org/documentation/installation/installing-images/) for examples.
- Install SD card in Raspberry Pi and configure as normal (enable Wifi, SSH, etc).

##### Build and Launch OpenThread Daemon

These instructions are to be performed on the Raspberry Pi.

- Install tools:

  ```sh
  sudo apt install libmbedtls-dev libbsd-dev autoconf libtool
  sudo apt install git
  ```

- Clone the appropriate repositories:

  ```sh
  mkdir br
  cd br
  git clone https://github.com/Abhayakara/openthread.git
  cd openthread/third_party/
  git clone https://github.com/Abhayakara/mdnsresponder.git
  cd ..
  ```

  ``` Note::
      These repositories are branches off of the OpenThread project. Here we have developed a border router with
      *srp-mdns-proxy* integration built in.
  ```

- Build the daemon:

  ```sh
  ./bootstrap
  HOST= make -f src/posix/Makefile-posix DAEMON=1 DEBUG=1
  ```

- Launch the daemon:

  ```sh
  sudo output/posix/armv7l-unknown-linux-gnueabihf/bin/ot-daemon 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200' &

  OT_CTL_SCRIPT="
    panid 0xabcd
    channel 11
    masterkey 00112233445566778899aabbccddeeff
    dataset commit active
    ifconfig up
    thread start
    networkname"

  sudo output/posix/armv7l-unknown-linux-gnueabihf/bin/ot-ctl "$OT_CTL_SCRIPT"
  ```

  ``` Note::
      The `OT_CTL_SCRIPT` above sets the Thread Network Credentials. These may be altered as you see fit.
  ```

At this point you may use `ot-ctl` to manage your Thread Network.
[OT-cli reference](https://github.com/openthread/openthread/blob/master/src/cli/README.md)

##### Build and Launch srp-mdns-proxy
This section presumes that the **Build and Launch OpenThread Daemon** steps have been performed. The step that clones
*mdnsresponder* into the third_party folder is particularly important.

- Install *mdnsresponder*. From the OpenThread directory, run:

```sh
cd third_party/mdnsresponder/mDNSPosix
sudo make os=linux
sudo make install
```

- Build *srp-mdns-proxy*:

```sh
cd third_party/mdnsresponder/ServiceRegistration
make
```

- Launch *srp-mdns-proxy*:

```sh
sudo ./build/srp-mdns-proxy --log-stderr
```

At this point srp-mdns-proxy should begin running. You should see debug output as the service runs.

#### Setting up the RCP
The following instructions explain how to build an RCP for a Nordic PCA10059 RCP. This section presumes you have
retrieved the openthread repo described in *Build and Launch OpenThread Daemon*

- Build the RCP Image. From the openthread root directory, run:

```sh
./script/bootstrap
./bootstrap
make -f examples/Makefile-nrf52840 USB=1 BOOTLOADER=USB
```

- Convert the output to a *.hex*:

```sh
 arm-none-eabi-objcopy -O ihex output/nrf52840/bin/ot-rcp ot-rcp.hex
```

- Use [nRF Connect](#flashing-nordic-devices) to flash the ot-rcp.hex file to the RCP dongle.

*The RCP dongle is now ready to be attached to the Border Router*

### Commissioning a Thread Device to the Thread Network
The Border router has now initiated a Thread Network. To connect a Thread Device to the Thread Network you must provide
the device with Thread Network Credentials. This may be done by:

- *Commissioning the accessory over HAP*: If the accessory supports BLE it may be paired over BLE and commissioned.
through the ThreadManagementControl characteristic. See the Thread specification for details.
- *Static Commissioning*: The ADK may be built with pre-determined Thread Network Credentials.
- *Joiner Mode*: The ADK may be built such that it boots into 'Joiner Mode'. An accessory in Joiner Mode must be
manually commissioned by a node on the Thread Network (In this case the Border Router).
See [OpenThread](https://openthread.io/guides/build/commissioning) for details.

The default build made from *make TARGET=nRF52 PROTOCOLS=THREAD* requires either commission over BLE or joiner mode
triggered over BLE. If ADK Thread device is built with *make USE_BLE=0 TARGET=nRF52 PROTOCOLS=THREAD*, it will be
built as a Thread-only device and will use joiner mode. When built with
*make TARGET=nRF52 PROTOCOLS=THREAD USE_STATIC_COMMISSIONING=1*, the accessory will be boot with hard-coded
commissioning credentials.

#### Commissioning a Thread Device over HAP
- Build a standard Thread Accessory:

  ```sh
  make TARGET=nRF52 PROTOCOLS=THREAD
  ```

- Load the Accessory. Note both *Thread* and *BLE* are enabled:

   ```sh
   ./Tools/install.sh \
     -d nRF52 \
     -a Output/nRF52-arm-none-eabi/Debug/THREAD/Applications/Lightbulb.Oberon \
     -k \
     -t thread \
     -t ble
   ```

- Pair the accessory with HAT over BLE:
- Using HAT, write the following TLV to the *Thread Control Point* characteristic of the *Thread Transport* service:

```sh
0101010230010A4F70656E54687265616402020B000302CDAB0408DEAD00BEEF00CAFE051000112233445566778899AABBCCDDEEFF
```

This command breaks down as follows:

- *0101010230010A4F70656E5468726561640202* - Header
- *0B00* - Channel Number (11)
- *0302* - Header
- *CDAB* - PANID (0xABCD)
- *0408* - Header
- *DEAD00BEEF00CAFE* - ExtPANID
- *0510* - Header
- *00112233445566778899AABBCCDDEEFF* - Master Key

``` Note::
    Other command TLVs based on Thread Management service specification can also be written to the *Thread Control
    Point* for other purposes, for example:

    - *010102*: ClearParameters command
    - *010104*: Initiate joiner command
```

#### Commissioning a Thread Accessory Statically
The ADK may be built to launch "just knowing" its Thread Network Credentials. This option must be used only for the
convenience of testing the device over Thread without having to go through either commissioning over BLE or Thread
joiner mode.

- Update the ADK Static Commissioning parameters. These are set in
  *ADK/PAL/Thread/OpenThread/HAPPlatformThreadUtils+Commissioning.c*:

   ```c
   #ifndef THREAD_PANID
   #define THREAD_PANID 43981
   #endif
   #ifndef THREAD_EXTPANID
   #define THREAD_EXTPANID 0xDEAD00BEEF00CAFEull
   #endif
   #ifndef THREAD_CHANNEL
   #define THREAD_CHANNEL 11
   #endif
   #ifndef THREAD_MASTERKEY_UPPER64
   #define THREAD_MASTERKEY_UPPER64 0x0011223344556677ull
   #endif
   #ifndef THREAD_MASTERKEY_LOWER64
   #define THREAD_MASTERKEY_LOWER64 0x8899AABBCCDDEEFFull
   #endif
   ```

  ``` Note::
      PANID is decimal, not hex. *43981* corresponds to *0xabcd*
  ```

- Compile the ADK for static commissioning thread:

  ```sh
  make TARGET=nRF52 PROTOCOLS=THREAD USE_STATIC_COMMISSIONING=1
  ```

- Load the appropriate ADK accessory onto the PCA10056 device (Nordic Dev Board):

  ```sh
  ./Tools/install.sh \
    -d nRF52 \
    -a Output/nRF52-arm-none-eabi/Debug/THREAD/Applications/Lightbulb.Oberon \
    -k \
    -t thread
  ```

- Make a note of the Device ID and IP address. In the log you will see something like the following:

  ```sh
  1.173	Default	Thread state changed! Flags: 0x00000001 Current role: child.

  1.175	Debug	FDDE:AD00:BEEF:0000:0000:00FF:FE00:4002
  1.176	Debug	FDDE:AD00:BEEF:0000:0F83:FB05:77CC:100F
  1.177	Debug	FE80:0000:0000:0000:800F:1D7B:D24C:68A5
  ```

  ``` Note::
      - *Current role: child*. This indicates that the device has joined a Thread Network.
      - *1.174 Debug FD11:0022:0000:0000:18F4:7861:CE3F:1E1E*. This is the first IP address listed after the role
        change. This IP address is the one exposed beyond the Thread network and is the one your MacBook will
        communicate with.
  ```

- Verify the border router can communicate with the thread device. Ping the thread device from the border router:

    ```sh
    ping6 FD11:0022:0000:0000:18F4:7861:CE3F:1E1E
    ```

- Verify the MacBook can ping the thread device:

    ```sh
    ping6 FD11:0022:0000:0000:18F4:7861:CE3F:1E1E
    ```

- If the ping fails, it is possible that the Mac does not know to route the ping request through the Border Router. Run
  the following command on the RPi to create a static route and try again:

    ```sh
    sudo ip -6 route add fd11:33::/64 dev wlan0 table local
    ```

- It is also possible that MacOS has failed to retrieve the route from the RPi border router. Add a static route to your
border router for fd11:22:/64. For example, if *fe80::ba27:3bff:fe67:64fc* is the border router's wlan address then:

    ```sh
    sudo route add -inet6 fd11:22::/64 fe80::ba27:ebff:fe67:64fc%en0
    ```

#### Commissioning a Thread Device using Joiner Mode
When the ADK is built without Static Commissioning or BLE support it will automatically boot into
[Joiner Mode](https://openthread.io/guides/build/commissioning). Joiner Mode is when a Thread Accessory is actively
searching for a Thread Network to join. The Thread Network must be told to accept a Joiner.

- Build the ADK without BLE support:

  ```sh
  make TARGET=nRF52 PROTOCOLS=THREAD USE_BLE=0
  ```

- Load the ADK without BLE support:

  ```sh
  ./Tools/install.sh \
    -d nRF52 \
    -a Output/nRF52-arm-none-eabi/Debug/THREAD/Applications/Lightbulb.Oberon \
    -k \
    -t thread
  ```

- The accessory will boot into Joiner Mode. It will periodically advertise its `EUI` and `Joiner Passphrase`. Make a
note of these:

   ```sh
       5.029	Debug	EUI: F4:CE:36:6B:04:F7:D4:DF
       5.029	Debug	Joiner Passphrase: 6D8E3F0A99C2D399A334619304800804
   ```
- Launch `ot-ctl` at the border router. From the OpenThread root directory:

   ```sh
   sudo output/posix/armv7l-unknown-linux-gnueabihf/bin/ot-ctl
   ```

- Initialize the commissioner:

   ```sh
   > commissioner start
   Commissioner: petitioning
   Done
   > Commissioner: active
   ```

- Tell the commissioner to accept the joiner with EUI and Passphrase:

   ```sh
   > commissioner joiner add <EUI> <PASSPHRASE>
   Done
   ```

   For example:

   ```sh
   > commissioner joiner add F4CE366B04F7D4DF 6D8E3F0A99C2D399A334619304800804
   Done
   ```

  ``` Note::
      You may replace the EUI with `*` to accept all joiners with the appropriate passphrase.
  ```

- After a few minutes the accessory will be allowed into the Thread Network by the Border Router, receive its
commissioning credentials, and register with *srp-mdns-proxy*.

## Tools and Notes

### Flashing Nordic Devices

Nordic provides different tools for flashing their devices depending on the file used to flash.
- To flash .hex files use nrfConnect
- To flash .zip use nrfutil.

#### nrfConnect
You can download nrfConnect from [here](https://www.nordicsemi.com/?sc_itemid=%7B49D2264D-62FD-4C16-811F-88B477833C5D%7D).

- From the programmer app, on the top right corner choose *Add HEX file* and select the target `*.hex` image.
- Insert the USB dongle and press the button sticking out on top of the metallic box next to the big white button
(not the big white button itself) to enter flash mode (the led will flash red).
- From the programmer app, go to the upper left corner and select the device from the drop down menu.
- Select *Write* on the lower right corner of the programmer app.

#### nrfutil
Install *nrfutil* with the following command in your python bin directory.

```sh
sudo easy_install pip && pip install --user nrfutil
```

You may need to add that to your path or move *nrfutil* to an appropriate location

- Insert the USB dongle and press the button sticking out on top of the metallic box next to the big white button
(not the big white button itself) to enter flash mode (the led will flash red)

- Find the device

  ```sh
  ls /dev/tty.usbmodem*
  ```

- Type the following command:

  ```sh
  nrfutil dfu usb-serial -pkg <.zip image file path> -p <path to the device from previous step>
  ```

  For example:
  ```sh
  nrfutil dfu usb-serial -pkg <.zip image file path> -p /dev/tty.usbmodemDDF59963F0211
  ```
