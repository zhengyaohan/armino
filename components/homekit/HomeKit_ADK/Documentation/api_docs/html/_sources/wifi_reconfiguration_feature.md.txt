WiFi Reconfiguration Service
============================

This document describes the WiFi reconfiguration feature provided by the ADK and how to integrate it into a HomeKit
accessory. This feature enables an admin controller to reconfigure an accessory that is already configured on a WiFi
network to a new WiFi network.

## Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK. Once all the
prerequisites are installed for your platform, run the following to compile ADK with WiFi reconfiguration feature:

``` tabs::

   .. group-tab:: macOS

      .. Warning::
          This feature is currently not supported on this platform.

   .. group-tab:: Linux

      .. Warning::
          This feature is currently not supported on this platform.

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       make TARGET=Raspi USE_WAC=1 HAP_WIFI_RECONFIGURATION=1

   .. group-tab:: Nordic nRF52

      .. Warning::
          This feature is currently not supported on this platform.

```

``` Note::
    - *USE_WAC* should be used in combination with *HAP_WIFI_RECONFIGURATION* as there is some dependency on WAC code.
    - WAC requires ADK application to be configured for Hardware or Software Authentication. Please read
      :doc:`Accessory Authentication </accessory_authentication>` for more details.
```

## Run
Please follow instructions at [Getting Started](getting_started.md) page to run an ADK sample application.

## Overview
There are two ways to update the accessory - simple update and fail safe update.

In simple update, the controller sends new WiFi credentials (ssid and psk) to the accessory and ADK reconfigures the
accessory to the given WiFi network.If the WiFi credentials are correct, the accessory is successfully configured to the
network. If the WiFi credentials are incorrect, the accessory will be left in that state (no rollbacks or anything). For
most real-world use cases, we will not use simple update.

Similarly, in fail safe update, controller sends new WiFi credentials (ssid and psk) to the accessory and ADK reconfigures
the accessory to the given WiFi network with some fail safe provisions. If the WiFi credentials are correct, ADK will
successfully configure the accessory. If Ethernet is our current transport, ADK will wait for the accessory to connect
to the new network and obtain an IP address and if everything works fine, we will set the appropriate success flags in
the update status and be done. If the WiFi reconfiguration fails to happen, we will wait for the fail safe timeout
and then rollback the WiFi configuration to the previous configuration and set the respective error flags in the update
status.

If WiFi is our current transport, ADK will wait for the controller to connect to the accessory over the new WiFi
network and send a commit message within fail safe update timeout after the initiation of fail safe update, which should
have the same cookie value as the cookie value passed in the fail safe update. ADK will then set the appropriate success
flags in the update status and send a write response to the controller indicating that fail safe update was successful.
If the commit message never arrives within the fail safe timeout, ADK will consider that a failure and will
rollback the configuration to the previous WiFi credentials.

**Key requirements**

- Accessory must support a new WiFi Transport service with a control point characteristic - WiFi Configuration control
- Writes to the control point will trigger the four different types of operations - read configuration,
write (simple and fail safe update) configuration and a fail safe commit configuration.
- Accessory must use the wpa supplicant control interface and use the network information configured in the
`wpa_supplicant.conf` to connect to the WiFi interface.

## ADK Implementation

Currently the reference implementation for WiFi reconfiguration feature is only supported for Raspberry Pi (Linux)
targets.

Service(s) and characteristics(s) added to ADK -

- Add a new WiFi Transport service with 3 characteristics - Current Transport, WiFi Capability and WiFi Configuration
Control.
- Current Transport - If this is read from the accessory over a WiFi interface, it is set to true. If it is read over
Ethernet, this characteristic is set to false.
- WiFi Capability - This characteristics is a bit mask which indicates whether the accessory supports 2.4 GHz, 5 GHz,
wake on WLAN and station mode.
- WiFi Configuration Control - This is a control point characteristics and only admin controller can write to it which
will trigger the four different types of operations - read configuration, write (simple and fail safe update)
configuration and a fail safe commit configuration. The controller writes a TLV to the control point that contains
the operation type which is required for write requests, a cookie value which is required for write requests and
commit request, country code which is optional and station config which contains the WiFi credentials - ssid,
security mode(open network or WPA2-PSK) and psk (Not present for open network),

### Implementation details

- Add service and characteristics as specified in the section above to ADK
- Write a HAP request handler to handle reads on the 3 characteristics specified above and also handles writes on the
control point characteristics.
- Support read configuration - Read the `wpa_supplicant` conf file and get the persisted cookie, update status and the
network configuration (if any). Send a write response with all of the above values (set psk to empty string to indicate
presence of a PSK but not share the actual data for security reasons)
- Support write configurations -

#### Simple update
- Write to the `wpa_supplicant` conf file using the existing HAPPlatformWiFiManagerApplyConfiguration which was modified
to add a cookie value and update status. Update status is defaulted to 0.
- Restart the WiFi interface.

#### Fail safe update
##### WiFi as the current transport
- Decode and validate the TLV for the write request. Start a fail safe timeout timer. The fail safe timeout value can be
passed in from the controller as part of the request. Default is 60 secs.
- Send a resource busy error if a fail safe update is already pending.
- If we are on WiFi as the current transport, we would need to restart the accessory for the changes to take
effect. In such a case, we set the restart required flag in the update status.
- We also set the update pending flag in the status so that if there are multiple requests, we can check if
an update is pending and return a resource busy error to other controllers.
- Back up the existing configuration that is present in `wpa_supplicant.conf` to `wpa_supplicant.conf.orig` file
- Write to the `wpa_supplicant` using the modified `HAPPlatformWiFiManagerApplyConfiguration` api.
- Wait for the controller to send a commit message with the same cookie value and over the new WiFi
interface before the fail safe timeout timer expires. Set the update success flag, reset all
the other flags set in update status and respond to the commit message.
- If the commit message is not received when the fail safe timeout timer expires, rollback the WiFi
credentials to that stored in the `wpa_supplicant.conf.orig` file. Update the cookie value in the
`wpa_supplicant.conf` and clear the update pending and restart required flag. Set the update failed flag and
set any appropriate flags like link established, authentication failed, etc. so the controller can use it
for debugging if needed
- If the commit message is received after the fail safe timeout has expired with the correct cookie value and
over a WiFi interface, respond to the request with the persisted cookie value and the update status.

##### Ethernet as the current transport
- Decode and validate the TLV for the write request. Start a fail safe timeout timer. The fail safe timeout value can be
passed in from the controller as part of the request. Default is 60 secs.
- Send a resource busy error if a fail safe update is already pending.
- If we are on Ethernet as the current transport, we set the update pending flag in the status so that if
there are multiple requests, we can check if an update is pending and return a resource busy error to other
controllers.
- Back up the existing configuration that is present in `wpa_supplicant.conf` to `wpa_supplicant.conf.orig` file
- Write to the `wpa_supplicant` using the modified HAPPlatformWiFiManagerApplyConfiguration api.
- Poll the `wpa_cli` status interface to check if we are connected to the new WiFi configured in step e. and
also obtain a valid IP address on the WiFi interface and set the appropriate flags in the update status.
- If we are successfully connected to the WiFi interface, we clear all other flags in the update status and
set the update success flag.
- If we failed to connect to the WiFi interface or the fail safe timeout timer expires,
rollback the WiFi credentials to that stored in the `wpa_supplicant.conf.orig` file. Update the cookie value
in the `wpa_supplicant.conf` and clear the update pending and restart required flag. Set the update failed
flag and set any appropriate flags like link established, authentication failed, etc. so the controller
can use it for debugging if needed.

### HAP Modules

#### Source files
- `HAPServiceTypes.c` and `HAPServiceTypes.h`
- `HAPCharacteristicTypes+TLV.c` and `HAPCharacteristicTypes+TLV.h`
- `HAPCharacteristicTypes.c` and `HAPCharacteristicTypes.h`
    - Service and characteristics information, TLVs and structs needed.
- `HAPRequestHandlers+WiFiReconfiguration.c` and `HAPRequestHandlers+WiFiReconfiguration.h`
    - Request handler for handling reads and writes for all the 3 characteristics
    - Has the core logic of this feature and calls PAL APIs from WiFi manager to accomplish the above

### PAL Modules
- `HAPPlatformTCPStreamManager.c` and `HAPPlatformTCPStreamManager.h`
   - APIs added for detecting if WiFi or Ethernet is the current transport and also the WiFi capabilities of the
   accessory
      - HAPPlatformTCPStreamManagerIsWiFiCurrentTransport - Returns true if the current transport used by the socket
      of the receive TCP stream manager instance is WiFi. False otherwise.
      - HAPPlatformTCPStreamManagerGetWiFiCapability - Returns the supported Wi-Fi capabilities.

- `HAPPlatformWiFiManager.c` and `HAPPlatformWiFiManager.h`
    - Added APIs for saving the WiFi credentials to the `wpa_supplicant`, getters and setters for the cookie value, update
      status, getters for ssid and to check if psk is configured, backup the configuration to `wpa_supplicant.conf.orig`,
      restore the WiFi credentials from the backed up configuration and clear the configuration.
      - HAPPlatformWiFiManagerGetCookie - Get the cookie value persisted in the wpa supplicant.
      - HAPPlatformWiFiManagerSetCookie - Set the cookie value to be persisted in the wpa supplicant.
      - HAPPlatformWiFiManagerGetUpdateStatus - Get the updateStatus value persisted in the wpa supplicant.
      - HAPPlatformWiFiManagerSetUpdateStatus - Sets the updateStatus value in the wpa supplicant to the new value.
      - HAPPlatformWiFiManagerIsPSKConfigured - Checks if PSK is configured in the wpa supplicant.
      - HAPPlatformWiFiManagerBackUpConfiguration - Backs up the Wi-Fi network configuration.
      - HAPPlatformWiFiManagerIsWiFiLinkEstablished - Parses the wpa_status result returned using the wpa_cli status
      command and returns true if wpa state is connected.
      - HAPPlatformWiFiManagerIsWiFiNetworkConfigured - Parses the wpa_status result returned using the wpa_cli status
      command and returns true if ip address is obtained.
      - HAPPlatformWiFiManagerClearConfiguration - Clear the configuration and remove all network configurations (Used
      by WAC code as well)
    - Modified APIs
      - HAPPlatformWiFiManagerApplyConfiguration - Added parameters cookie value, update status and a flag to
      indicate if we should restart the WiFi interface or not.
      - HAPPlatformWiFiManagerRestartWiFi - Using wpa_cli reconfigure command to restart the WiFi interface.

### Testing WiFi Reconfiguration

We can test some basic use cases for Wifi Reconfiguration by using the HAT tool. Below are the instructions on how
to do so.
- Run the Lightbulb accessory after compiling it with the appropriate flags to enable the WiFi reconfiguration
feature.
- Open the Home Accessory Tester tool and Discover the services and characteristics of the Lightbulb accessory.
- Navigate to the Wi-Fi Configuration Control characteristics on the left pane.
- Go to Prepare and Execute Timed Write [tlv8] on the right side. Click on Build TLV.
- You can choose the Operation Type - Read, Simple Update, Fail Safe Update and Commit for fail safe update.

#### Different use cases to test
- Read configuration - Nothing else needs to be filled in. Click Build TLV. That should populate the Prepare and
Execute Timed Write [tlv8]. Click on Timed Write. On the terminal running the accessory, you should be able to
see the current WiFi configuration.
- Simple update configuration - Need to put in a cookie value that should be unique for this request, country code
e.g. - "US", SSID, Choose the security mode, if WPA-PSK is chosen, either enter the plain text passphrase or
PSK. Click Build TLV. That should populate the Prepare and Execute Timed Write [tlv8]. Click on Timed Write.
On the terminal running the accessory, you should be able to see the new WiFi Reconfiguration get applied.
- Fail safe update configuration (Ethernet as current transport) - Need to put in a cookie value that should be
unique for this request, country code e.g. - "US", SSID, Choose the security mode, if WPA-PSK is chosen, either
enter the plain text passphrase/PSK. Click Build TLV. That should populate the Prepare and Execute Timed Write
[tlv8]. Click on Timed Write. On the terminal running the accessory, you should be able to see the new WiFi
Reconfiguration get applied. If your WiFi credentials are incorrect or network is unreachable or a failure
happens, we will rollback to the old valid WiFi credentials.
- Fail safe update configuration (WiFi as current transport) - Configure the Raspberry pi on a WiFi network and
disconnect the Ethernet cable. Make sure you can see the Lightbulb advertising on Bonjour.
Need to put in a cookie value that should be unique for this request, country code e.g. - "US", SSID, Choose the
security mode, if WPA-PSK is chosen, either enter the plain text passphrase/PSK. Click Build TLV. That should
populate the Prepare and Execute Timed Write [tlv8]. Click on Timed Write. On the terminal running the accessory,
you should be able to see the new WiFi Reconfiguration get applied. For WiFi as current transport, we need to
send the Commit message within 30 secs of sending the fail safe update request. As soon as the accessory comes
back on HAT on the new network, choose the Commit configuration operation type and choose the cookie value that
matches the one sent for the fail safe update. Only then the WiFi configuration will succeed otherwise it will
fail. If your WiFi credentials are incorrect or network is unreachable or a failure happens, we will rollback to
the old valid WiFi credentials.
