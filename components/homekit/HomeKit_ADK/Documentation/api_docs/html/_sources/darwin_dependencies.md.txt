ADK dependencies for macOS
===========================

Following are the third party libraries ADK uses for development on macOS. We recommend using
[BREW](https://brew.sh) package manager to install these dependencies.

## Mandatory
- *openssl@1.1* - Used as a default crypto library.
- *pkg-config* - Used to get the header and link library paths for third party libraries.

## Optional
- *wget* - Used by various ADK tools.
- *docker* - Used to compile ADK for Linux or Raspberry Pi.
- *ccache* - Used to speed up compilation time for macOS ADK applications.
- *hidapi* - Used to enable support for HW Auth on macOS.
- *qrencode* - Used to display QRCode when dynamic display support is enabled on macOS.
- *libnfc* - Used to enable support for NFC on macOS.
- *mbedtls* - Used to enable the use of MBedTLS crypto library instead of the default of OpenSSL.

