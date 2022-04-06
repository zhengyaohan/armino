HomeKit Bridge
==============

## Overview
A HomeKit bridge enables HomeKit controllers to access non-HomeKit accessories. Without a bridge HomeKit establishes
end-to-end security with a HomeKit controller being one of the endpoints and an accessory being the other endpoint.
However, with a HomeKit bridge the bridged accessories by definition do not support HAP and so the bridge becomes the
endpoint of the secure connection. To mitigate this fact, HomeKit defines restrictions on the profiles and protocols
that may be supported by a bridge. See the *HomeKit Accessory Protocol Specification* for more details. A HomeKit bridge
itself is also a HomeKit accessory and must be an *IP* accessory (Wi-Fi or Ethernet).

### Compile
You must have all the [prerequisites](getting_started.md) for your development platform to compile the ADK.
Once all the prerequisites are installed for your platform, run the following to compile the sample Lightbulb Bridge
application:

``` tabs::

   .. group-tab:: macOS

      .. code-block:: bash

       make TARGET=Darwin APPS="Bridge"

   .. group-tab:: Linux

      .. code-block:: bash

       make TARGET=Linux APPS="Bridge"

   .. group-tab:: Raspberry Pi

      .. code-block:: bash

       make TARGET=Raspi APPS="Bridge"

   .. group-tab:: Nordic nRF52

      .. code-block:: bash

       make TARGET=nRF52 APPS="Bridge"

```

### Run
Please follow instructions at [Running an ADK Application](getting_started.html#step-5-running-an-adk-application) to
run an ADK sample application.

## Implementation Details

### Accessory Attribute Database
A bridge must represent the accessory attribute databases of the accessories that it currently bridges to, as separate
HAP accessory objects. A bridge may define an upper limit of HAP accessory objects (at most 150, according to the
specification). Sufficient memory for this number of accessories should be statically allocated, to ensure robust
operation. In the samples, this is done in App.c like this:

```c
static AccessoryState accessoryState[kAppState_NumLightBulbs];
```

The maximum number of bridged light bulbs is defined as the constant `kAppState_NumLightBulbs` in file *DB.h*.
See *ADK/Applications/Bridge* for more details of bridges for LightBulb-compatible accessories, and
*ADK/Applications/RemoteBridge* for Remote accessories.

The amount of memory that needs to be allocated depends on the accessory attribute databases of the bridged accessories,
i.e., on their profiles, more exactly on the number of attributes that have to be represented. For a simple light bulb,
the number of attributes is calculated like this (see *ADK/Applications/LightBulb/DB.h*):

```c
kAttributeCount = 10 + 3 + 5 + 4
```

i.e., the sum of the number of attributes for the *Accessory Information service (10)*,
*Protocol Information service (3)*, *Pairing service (5)*, and the *LightBulb service (4)*. For bridges, each bridged
accessory requires another *Accessory Information service* plus the number of attributes for the bridged accessories
themselves:

```c
kAttributeCount = 10 + 3 + 5 + (8 + 4) * kAppState_NumLightBulbs
```

Note that the bridged accessory itself exposes the *ADKVersion* and *Product Data characteristics* - as it is
implemented using the ADK - while the bridged light bulbs do not - as they are not implemented using the ADK. This
explains why it is *(8 + 4)* and not *(10 + 4)* in the expression above.

### Bridge Reconfiguration
Sometimes, bridges need to be reconfigured, e.g., for adding another bridged accessory. In such a case, the HomeKit
accessory server must be stopped using function `HAPAccessoryServerStop`. Once the accessory server has stopped, the
callback `HandleUpdatedState` from `HAPAccessoryServerCallbacks` is called. Then the list of bridged accessories is
changed (note that the list must be null-terminated), and then the accessory server must be started again using function
`HAPAccessoryServerStartBridge` with argument configurationChanged set to true. See also sections
[Start the Application](accessory_development.html#start-the-application) and [Terminate the Application](accessory_development.html#terminate-the-application).
The *config number (CN)* is incremented automatically if the configurationChanged argument is set to true for the
functions starting a bridge (`HAPAccessoryServerStartBridge` and `HAPAccessoryServerStartCameraBridge` respectively).

### Callbacks for Bridged Characteristics
When the HAP Library invokes a callback function in order to read or write a characteristic value, then the callback
must return immediately. It must never wait for a bridged accessory that is connected via a slow communication channel.
This means that for communication technologies such as ZigBee, the characteristic values must be cached.

In a read callback, the cached value of the characteristic is returned along with `kHAPError_None`. A cached value
should be returned even if it is not known to be completely up-to-date. The cache is then updated as fast as the
communication channel allows. If the previously cached value is not valid anymore and needs to be replaced with a new
value, the HomeKit controller(s) should be notified of the new value by calling `HAPAccessoryServerRaiseEvent`.

In a write callback, the cached value of the characteristic is updated and `kHAPError_None` returned. If the new value
is different from the previously cached value, the bridged accessory is updated as fast as the communication channel
allows. If the bridged accessory signals that this update failed, the cache should be reverted to the old value and the
HomeKit controller(s) should be notified of the old value by calling `HAPAccessoryServerRaiseEvent`.

When a bridged accessory is known to be currently unreachable, this can be signaled in a read or write handler by
returning `kHAPError_Unknown` (*unable to communicate with requested service*).

For a HAP multi-characteristic write, the ADK invokes the callbacks of all involved characteristics in one go, meaning
that any timer or other callbacks scheduled to the run loop will not execute before all pending characteristic write
callbacks have been invoked.

### AIDs of Bridged Accessories
For bridged accessories, it must be ensured to assign new accessories a new AID and not to reuse an old AID. Otherwise
some things such as Remote profile don't work correctly.

Example of *App.c* from *RemoteBridge*:

```c
/**
 * Compute a new accessory instance ID.
 */
HAP_RESULT_USE_CHECK
static uint64_t GetNewAccessoryInstanceID(void) {
    uint64_t aid;
    bool collision;
    do {
        HAPPlatformRandomNumberFill(&aid, sizeof aid);
        aid &= 0x7FFFFFFFFFFFFFFF; // Workaround for certain controllers that have issues with larger aid values.
        collision = false;

        if (aid == 1 || aid == 0) {
            continue;
        }

        for (size_t i = 0; i < kAppState_NumRemotes; i++) {
            if (aid == accessoryConfiguration.state.bridgedAccessory[i].aid) {
                collision = true;
                break;
            }
        }
    } while (collision);
    HAPAssert(aid > 1);
    return aid;
}
```
