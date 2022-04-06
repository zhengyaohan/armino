Troubleshooting ADK
===================

<details>
  <summary>Homebrew Installation Error</summary>
  <br>

  Homebrew requires Xcode Command Line Tools to be installed in `/Library/Developer/CommandLineTools`, which requires
  installing the CLT from the [Apple developer website](https://developer.apple.com/download/more/). If Homebrew
  attempts to install the CLT and fails with a `Can't install the software because it is not currently available from
  the Software Update server` error, it may indicate CLT is already installed but not in the expected location.

  **Solution:**
  * Reinstall the Command Line Tools from the link above and verify it is installed to
  `/Library/Developer/CommandLineTools`.
  * If that fails, make a temporary symbolic link from `/Library/Developer/CommandLineTools` to your Xcode installation
  (check `xcode-select -p`) for the duration of the Homebrew installation.
  * If all else fails, download the Homebrew install script and modify it to skip CLT installation.

</details>
<br>

<details>
  <summary>Build/Makefile: Operation not permitted</summary>
  <br>

  MacOS enabled new user privacy protections since version 10.15 that doesn't permit access to `~/Documents`, `~/Desktop`
  and `~/Downloads` directories. More details can be found at
  [A Guide to Catalina’s Privacy Protection](https://eclecticlight.co/2020/01/16/a-guide-to-catalinas-privacy-protection-3-new-protected-locations/).

  **Solution:**
  Move ADK code out of `~/Documents`, `~/Desktop`, or `~/Downloads` directories and try again.

</details>
<br>

<details>
  <summary>Make command seems to hang</summary>
  <br>

  On macOS, sometimes the ADK builds that use Docker (Linux/Raspi/nRF52) seem to stall as they are starting.

  **Solution:**
  Go to Docker Preferences -> General and enable the option to `Use gRPC FUSE for file sharing`.

</details>
<br>

<details>
  <summary>Failed to pair an accessory on macOS</summary>
  <br>

  If the ADK log doesn't show any activity after you have entered the pairing code in the Home app then check to make
  sure that the ADK is *not located* in either `~/Documents`, `~/Desktop`, or `~/Downloads` directories.

  **Solution:**
  Move ADK code out of `~/Documents`, `~/Desktop`, or `~/Downloads` directories and try again.

</details>
<br>

<details>
  <summary>Accessory not available for pairing in Home App</summary>
  <br>

  Checks To Perform:
  - All devices on the same network? -> iOS/tvOS/Raspberry Pi
  - Accessory unpaired? -> In ADK logs, look for "sf" value (1 = unpaired, 0 = paired). See below:

  ```sh
  2020-12-21'T'23:58:33.086506    Debug   [com.apple.mfi.HomeKit.Platform:ServiceDiscovery] txtRecord[6]: "sf"
    0000  31                                                                         1
  ```

  **Solution:**
  ``` tabs::

    .. group-tab:: macOS

        - In Terminal window 1, launch the sample ADK application ``./Output/Darwin-x86_64-apple-darwin$(uname -r)/Debug/IP/Applications/Lightbulb.OpenSSL``
        - In Terminal window 2, run ``killall -SIGUSR1 Lightbulb.OpenSSL``

    .. group-tab:: Raspberry Pi

      - In Terminal window 1, SSH into the Raspberry Pi and launch the sample ADK application ``sudo ./Lightbulb.OpenSSL``
      - In Terminal window 2, run ``sudo kill -SIGUSR1 `pidof Lightbulb.OpenSSL```

```
</details>
<br>

<details>
  <summary>ADK application crashes with Abort trap: 6 on macOS</summary>
  <br>

  **Solution:**
  ``` tabs::

    .. group-tab:: macOS

        Please make sure that your terminal application has *Bluetooth* access under
        *System Preferences* -> *Security & Privacy* -> *Privacy*

```
</details>
<br>
