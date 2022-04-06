Lock Profile
============

## Overview
The Lock profile defines a collection of services and characteristics to enable lock support in a HomeKit accessory.
This document describes the lock features in ADK.

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK. Once all the
prerequisites are installed for your platform, run the following to compile ADK with lock features:

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin PROFILE=Lock

   .. group-tab:: Linux

      .. Warning::
          This feature is currently not supported on this platform.

   .. group-tab:: Raspberry Pi

      .. Warning::
          This feature is currently not supported on this platform.

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       make TARGET=nRF52 PROFILE=Lock

```

### Run
Please follow instructions at [Running an ADK Application](getting_started.html#step-5-running-an-adk-application) to
run the Lock sample application.

## Implementation Details

### Services and features
The following services and features are used by the Lock profile:

- [Lock Mechanism Service](#lock-mechanism-service)
- [Lock Management Service](#lock-management-service)
- [Access Code Service](#access-code-service)
- [NFC Access Service](#nfc-access-service)
- [Event Notification Context](#event-notification-context)

#### Lock Mechanism Service
This service enables control of the physical lock mechanism.

#### Lock Management Service
This service enables vendor-specific actions as well as user management related functions.

#### Access Code Service
This service enables the access code feature. Using the Access Code Service, HomeKit controllers can persist access
codes onto the accessory. Users can lock and unlock using an access code that has been persisted.

##### Feature Flags
The following compile time feature flag enables the access code functionality in the Lock sample application:

- `USE_ACCESS_CODE`

##### App Implementation
- *Applications/Common/Helper/AccessCodeHelper.c*
    - This file contains code to handle the list/read/add/update/remove operation of one or more access codes. Bulk
      operation of the same type is allowed. For example, HomeKit controller can perform read on 5 access codes in one
      request.
- *Applications/Lock/App.c*
    - This file contains source code for initializing the Access Code platform.

##### HAP Implementation
- *HAP/HAPRequestHandlers+AccessCode.c*
    - This file contains code to handle read and write requests of Access Code Service characteristics.

##### Keypad Disable
The Lock sample application implements a mechanism to temporarily disable the keypad if too many invalid access codes
have been entered to lock or unlock the accessory. The *Active* characteristic is used to reflect the keypad being
enabled or disabled. The *Active* characteristic should be Active if the keypad is enabled and Inactive if it is
disabled.

The amount of time to disable the keypad is based on a sliding window of time from the current moment and extending
backward by either 10 minutes or 1 hour. There can be no more than 10 invalid attempts during a 10 minute window, and
no more than 20 invalid attempts during a 1 hour window. Once the keypad is disabled, it will automatically be
re-enabled when the sliding window no longer contains the specified maximum invalid attempts. For example:
- 9:00 - 1st invalid attempt
- 9:03 - 2nd invalid attempt
- 9:06 - 3rd-10th invalid attempts -> The keypad is disabled due to 10 invalid attempts in a 10 minute window
- 9:10 - Keypad is re-enabled -> There are 9 invalid attempts logged for the 10 minute window and 10 invalid attempts
  logged for the 1 hour window
- 9:15 - Two more invalid attempts -> The keypad is disabled due to 10 invalid attempts in a 10 minute window
- 9:16 - Keypad is re-enabled -> There are 2 invalid attempts logged for the 10 minute window and 12 invalid attempts
  logged for the 1 hour window
- 9:25 - There are zero invalid attempts logged for the 10 minute window and 12 invalid attempts logged for the 1 hour
  window
- 9:37 - Eight more invalid attempts -> The keypad is disabled due to 20 invalid attempts in a 1 hour window
- 10:00 - Keypad is re-enabled

The following can re-enable the keypad (if disabled) as well as reset invalid attempt counts:
- Physical key usage to modify lock state
- HAP request to modify the value of *Lock Target State* characteristic
- HAP request to modify the value of *Active* characteristic from Inactive to Active
- Button 3 press to toggle the lock state (on a platform that supports physical buttons)
- Valid access code entry (if keypad is enabled)

##### Bulk Operation
Any `Operation Type` that must provide `Access Code Control Request` may be performed in bulk. To perform a bulk
operation, more than one `Access Code Control Request` is included as part of the *Access Code Control Point*
characteristic. Only operations of the same type can be included in the bulk operation. The response to the write
request will include a corresponding `Access Code Control Response` for each request. The only exception to this is when
attempting to add more than the maximum allowed access codes. See example below.

The responses for a bulk operation may contain a mixture of success and failure status. A few examples:
- Bulk add: "1234", "1111", "1234", "5678", "222a" -> Response includes 5 `Access Code Control Response` with status of
  'Success', 'Success', 'Duplicate', 'Success', 'Invalid character'
- Bulk add 15 codes and the maximum allowed is 5 access codes -> Response includes 6 `Access Code Control Response` with
  status of 'Success', 'Success', 'Success', 'Success', 'Success', 'Exceeded maximum allowed access codes'

#### NFC Access Service
This service enables the NFC Tap-To-Unlock and Tap-To-Lock features. Using the NFC Access Service, HomeKit controllers
can provision an NFC enabled accessory with issuer key, device credential key (for expedited transactions), and reader
key. After successful provisioning, the accessory is ready for NFC lock features.

##### Feature Flags
The following compile time feature flags enable the NFC functionality in the Lock sample application:

- `USE_NFC_ACCESS`
- `HAP_KEY_EXPORT` - This feature flag enables exporting of HAP pairing long term public key which is used as the issuer
  key by default.

##### PAL Implementation
- *PAL/HAPPlatformNfcAccess.h*
    - This header file defines the following structures which represent the respective keys to be persisted to the NFC
      reader. Accessory vendors can modify these structures for platform specific needs:
        - `struct HAPPlatformNfcAccessIssuerKey`
        - `struct HAPPlatformNfcAccessDeviceCredentialKey`
        - `struct HAPPlatformNfcAccessReaderKey`

- The following APIs for list/add/remove of issuer key, device credential key, and reader key must be implemented to
  support NFC Access provisioning:
    - `HAPPlatformNfcAccessIssuerKeyList`
    - `HAPPlatformNfcAccessIssuerKeyAdd`
    - `HAPPlatformNfcAccessIssuerKeyRemove`
    - `HAPPlatformNfcAccessDeviceCredentialKeyList`
    - `HAPPlatformNfcAccessDeviceCredentialKeyAdd`
    - `HAPPlatformNfcAccessDeviceCredentialKeyRemove`
    - `HAPPlatformNfcAccessReaderKeyList`
    - `HAPPlatformNfcAccessReaderKeyAdd`
    - `HAPPlatformNfcAccessReaderKeyRemove`

##### HAP Implementation
- *HAP/HAPRequestHandlers+NfcAccess.c*
     - This file contains code to handle read and write requests of NFC Access Service characteristics.

##### App Implementation
- *Applications/Lock/App.c*
    - This file contains source code for initializing the NFC Access platform.

##### Issuer Keys
When a Home controller is paired with a lock that supports NFC Access Service, the long term public key of that
controller is automatically added as the issuer key for that user. When the controller is unpaired, the corresponding
issuer key is removed. To support non-Home users, issuer keys may be added/removed by performing a write request on the
*NFC Access Control Point* characteristic. Note that the issuer key added from a HAP pairing can only be removed by
unpairing the controller. Both types of issuer keys are persisted under the same key value store key. This means for
Home controllers, the same key is persisted twice, once with the HAP pairings and once with the issuer keys.

When the Lock sample application is powered on, the long term public key of existing HAP pairings is read and an attempt
is made to cache this key as an issuer key. For firmware updates where the previous version did not support NFC Access
service, this is to ensure that the long term public key of existing HAP pairings are added to the issuer key list.
Otherwise, this is to verify that previously added long term public keys have already been cached in the issuer key
list.

### Event Notification Context

The Event Notification Context feature represents the additional information that may be included as part of the event
notification for the following characteristics:
- Lock Current State
- Lock Target State

The additional information includes context identifier or the source that lead to the lock state to change. The Event
Notification Context feature is supported on all transports.

#### Feature Flags
The following compile time feature flag enables the additional event notification context feature in the Lock sample
application:

- `USE_LOCK_ENC`

## Debug Commands

More information about Debug Commands at [Debug Service](interact_with_applications.html#debug-service).

### Supported Commands

```sh
setCurrentState
    secured
    unsecured
    jammed
    unknown
toggleLockState
toggleLockJammed
batteryStatus
    normal
    low
```

- `setCurrentState [secured|unsecured|jammed|unknown]` changes the current lock state
- `toggleLockState` simulates lock/unlock
- `toggleLockJammed` simulates a lock that is unable to lock or unlock due to a physical problem
- `batteryStatus [normal|low]` changes the status low battery state

An ADK lock application built with the access code feature enabled (`USE_ACCESS_CODE=1`) has the following additional commands available:

```sh
accessCode
    keypad <value>
    supportedConfig
        characterSet <value>
        minimumLength <value>
        maximumLength <value>
        maximumAccessCodes <value>
```

- `accessCode keypad <value>` simulates access code entry on the keypad
- `accessCode supportedConfig characterSet <value>` changes the access code character set supported configuration value
- `accessCode supportedConfig minimumLength <value>` changes the access code minimum length supported configuration value
- `accessCode supportedConfig maximumLength <value>` changes the access code maximum length supported configuration value
- `accessCode supportedConfig maximumAccessCodes <value>` changes the access code maximum access codes supported configuration value

An ADK lock application built with the NFC feature enabled (`USE_NFC_ACCESS=1`) has the following additional commands available:

```sh
nfcAccess
    addDeviceCredential <key> <key_index> <issuer_key_identifier>
    supportedConfig
        maximumIssuerKeys <value>
        maximumSuspendedDeviceCredentialKeys <value>
        maximumActiveDeviceCredentialKeys <value>
```

- `nfcAccess addDeviceCredential <key> <key_index> <issuer_key_identifier>` adds a device credential key to the cache.  The associated issuer key must already exist in the cache. The key_index is the expected index the key should have been added as, e.g. if the key is the third key added, the index is expected to be 2.
- `nfcAccess supportedConfig maximumIssuerKeys <value>` changes the maximum issuer keys supported configuration value
- `nfcAccess supportedConfig maximumSuspendedDeviceCredentialKeys <value>` changes the maximum suspended device credential keys supported configuration value
- `nfcAccess supportedConfig maximumActiveDeviceCredentialKeys <value>` changes the maximum active device credential keys supported configuration value
