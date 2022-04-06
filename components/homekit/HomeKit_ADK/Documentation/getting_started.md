Getting started with ADK
========================
These steps will help you get started with building an ADK accessory.

- [Step 1 - Setting up a Development Platform](#step-1-setting-up-a-development-platform)
- [Step 2 - Setting up an Accessory Platform](#step-2-setting-up-an-accessory-platform)
- [Step 3 - Compiling an ADK Application](#step-3-compiling-an-adk-application)
- [Step 4 - Provisioning an ADK Application](#step-4-provisioning-an-adk-application)
- [Step 5 - Running an ADK Application](#step-5-running-an-adk-application)

### Step 1 - Setting up a Development Platform

Please make sure to setup your development platform, specifically macOS, even if you are targeting other accessory
platforms such as Raspberry Pi. This is needed because the tools required to provision the accessories are run on macOS.

``` tabs::

   .. group-tab:: macOS

      Please follow the steps below to install the required dependencies.

      - Download and install `Xcode <https://developer.apple.com/download/more/>`__. Minimum required version is
        *Xcode 11*.
      - Download and install `Command Line Tools <https://developer.apple.com/download/more/>`__ for the corresponding
        Xcode.
      - Set the path for the active developer directory ``sudo xcode-select -s <xcode_path>/Contents/Developer``.
      - List of macOS dependencies can be found at  :doc:`macOS ADK dependencies </darwin_dependencies>`.

      .. Important::
          Run docker (Look in Spotlight/Applications folder). Make sure you go to ``Docker → Preferences → General`` and
          check the option → ``Start Docker Desktop when you log in``. This needs to be done only once after installing
          Docker.

   .. group-tab:: Linux

      .. code-block:: bash

       sudo apt install docker

      .. Note::
          ADK uses `Docker <https://www.docker.com/>`__ by default to compile code on Linux. This can be turned off by
          setting ``DOCKER=0`` when compiling ADK.

   .. group-tab:: Raspberry Pi

       The Raspberry Pi development requires a macOS or a Linux machine. Please follow instructions for your
       respective development platform.

   .. group-tab:: Nordic nRF52

       The Nordic nRF52 development requires a macOS or a Linux machine. Please follow instructions for your
       respective development platform.

```

### Step 2 - Setting up an Accessory Platform

``` tabs::

   .. group-tab:: macOS

      Please follow the steps at `Setting up a Development Platform <#step-1-setting-up-a-development-platform>`__ to
      setup a macOS as an accessory platform.

   .. group-tab:: Linux

      If you already have setup your development platform then no additional steps are required to run a Linux ADK
      accessory.

   .. group-tab:: Raspberry Pi

      Please follow our detailed :doc:`Raspberry Pi Setup Guide </raspi_setup>` to get started with using a Raspberry
      Pi as an ADK accessory.

   .. group-tab:: Nordic nRF52

      HomeKitADK come with built in support for
      `Nordic nRF52840 <https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK>`__.

```

### Step 3 - Compiling an ADK Application

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin apps

      .. Note::
          Darwin accessory development is only supported on macOS platform.

   .. group-tab:: Linux

      .. code-block:: bash

       make TARGET=Linux apps

   .. group-tab:: Raspberry Pi

      ADK for Raspberry Pi can be compiled using Docker or directly on a connected Raspberry Pi.

      **Docker**

      .. code-block:: bash

       make TARGET=Raspi apps

      **Raspberry Pi**

      .. code-block:: bash

       ./Tools/build_on_target.sh -n <host_name> -p <password> -u <user_name> -c "TARGET=Raspi DOCKER=0 apps"

      .. Note::Note::
          Compiling using Docker is slower than compiling on Raspberry Pi directly.

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       make TARGET=nRF52 apps

```

### Step 4 - Provisioning an ADK Application

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       ./Tools/install.sh \
         -d macOS \
         -t <protocol> \
         -a Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/<protocol>/Applications/<app_name>.OpenSSL \
         -c <category> \
         -k \
         -i

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       ./Tools/install.sh \
         -d raspi \
         -t <protocol> \
         -a Output/Raspi-armv6k-unknown-linux-gnueabihf/Debug/<protocol>/Applications/<app_name>.OpenSSL \
         -n <host_name> \
         -u <user> \
         -p <password> \
         -c <category> \
         -k \
         -i

      - ``-n`` [Optional] argument can be provided to specify raspberry pi host name. Default is ``raspberrypi``.
      - ``-u`` [Optional] argument can be provided to specify raspberry pi user name. Default is ``pi``.
      - ``-p`` [Optional] argument can be provided to specify raspberry pi password. Default is ``raspberry``.

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       ./Tools/install.sh \
         -d nRF52 \
         -a Output/nRF52-arm-none-eabi/Debug/BLE/Applications/<app_name>.Oberon \
         -s <serial_number> \
         -c <category> \
         -k \
         -i

      - ``-s`` [Optional] argument can be provided to specify the serial number of a connected nRF52
        board if more than one nRF52 boards are connected to your host platform. The ``<serial number>`` is either
        printed on the boards or displayed on the selected device.

```

- `-t` [Optional] argument can be provided to specify the transport type. Possible values are *ip*, *ble* or
*thread*. Defaults to *ble* if not provided.
- `-c` [Optional] argument can be provided to specify the category of the device. Defaults to category *5* for
*Lightbulb*.
- `-k` [Optional] argument can be provided to create a key-value store with a default setup code of *111-22-333*.
Setting this option clears up the previous key-value store store. So, if you don't want to have to pair the application
again with Home app on your iOS device the next time you run the application please make sure to *not include* the
this argument.
- `-i` [Optional] argument can be provided to only generate the provisioning information but not run the application.

``` Note::
    Please run `./Tools/install.sh -h` to get more information about this tool.
```

### Step 5 - Running an ADK Application

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       ./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/<protocol>/Applications/<app_name>.OpenSSL

   .. group-tab:: Linux

      .. code-block:: bash

       ./Output/Linux-x86_64-pc-linux-gnu/Debug/IP/Applications/<app_name>.OpenSSL

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       ./Tools/install.sh \
         -d raspi \
         -a Output/Raspi-armv6k-unknown-linux-gnueabihf/Debug/<protocol>/Applications/<app_name>.OpenSSL \
         -n <host_name> \
         -u <user> \
         -p <password>

      - ``-n`` [Optional] argument can be provided to specify raspberry pi host name. Default is ``raspberrypi``.
      - ``-u`` [Optional] argument can be provided to specify raspberry pi user name. Default is ``pi``.
      - ``-p`` [Optional] argument can be provided to specify raspberry pi password. Default is ``raspberry``.

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       ./Tools/install.sh \
         -d nRF52 \
         -a Output/nRF52-arm-none-eabi/Debug/BLE/Applications/<app_name>.Oberon \
         -s <serial_number>

      - ``-s`` [Optional] argument can be passed to the script to specify the serial number of a
        connected nRF52 board if more than one nRF52 boards are connected to your host platform. The ``<serial number>``
        is either printed on the boards or displayed on the selected device.

```

### Advanced Options
Please refer to [ADK Compile Time Options](./make_options.md) to see a list of most common compile time options
available to compile ADK. Please refer to [HomeKit Services](index.html#homekit-services) section to see compile time
options available to compile specific HomeKit Services and Profiles.
