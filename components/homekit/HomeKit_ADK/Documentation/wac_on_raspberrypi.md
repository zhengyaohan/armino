WAC on Raspberry Pi
====================
Please follow the steps below to build, install and test WAC on a Raspberry Pi.

Pre-requisites:
- [Step 1 - Compiling ADK Application with WAC](#step-1-compiling-adk-application-with-wac)
- [Step 2 - Setting up Authentication](#step-2-setting-up-authentication)
- [Step 3 - Configuring accessory using WAC](#step-4-configuring-accessory-using-wac)

### Step 1 - Compiling ADK Application with WAC
Please go through the [Getting started with ADK](./getting_started.md) before following the instructions below.

<details>
<summary>Compiling with hardware authentication</summary>
<BR>

ADK for Raspberry Pi can be compiled using Docker or directly on a connected Raspberry Pi. To test WAC, we must use a
form of authentication. Either hardware or software authentication can be used.

**Docker**

```sh
make TARGET=Raspi USE_WAC=1 USE_HW_AUTH=1 apps
```

**Raspberry Pi**

```sh
./Tools/build_on_target.sh -n <host_name> -p <password> -u <user_name> -c "TARGET=Raspi DOCKER=0 USE_WAC=1 USE_HW_AUTH=1 apps"
```
</details>
<BR>

<details>
<summary>Compiling with software authentication</summary>
<BR>

**Docker**

```sh
make TARGET=Raspi USE_WAC=1 apps
```

**Raspberry Pi**

```sh
./Tools/build_on_target.sh -n <host_name> -p <password> -u <user_name> -c "TARGET=Raspi DOCKER=0 USE_WAC=1 apps"
```

``` Note::
    Compiling using Docker is slower than compiling on Raspberry Pi directly.
```
</details>
<BR>

### Step 2 - Setting up Authentication
Please refer to [Accessory Authentication](./accessory_authentication.md) for different Authentication types supported by ADK.

### Step 3 - Configuring accessory using WAC
<details>
  <summary>Using HAT</summary>
<BR>

If the accessory has a previous configuration, either factory reset the accessory or just delete the network block in
`/etc/wpa_supplicant/wpa_supplicant.conf` and restart the raspberry pi.

- Pair the accessory with HAT using the set up code used to run the application.
- Make sure accessory is advertising on _hap._tcp. You can use the Discovery app for macOS available on the mac app store.
- Use Wi-Fi Accessory Configuration (WAC2) section in HAT to populate the Wifi SSID and PSK.
- Click the Send WAC configuration button.
- Wait until the accessory is connected to HAT again to use the accessory.

</details>
<BR>

<details>
  <summary>Using iOS</summary>
<BR>

- Go to Home App on iOS.
- Make sure accessory is advertising on _hap._tcp. You can use the Discovery app for macOS available on the mac app store.
- Click on + on the top right corner. Click Add Accessory.
- Click More options.
- It will show all accessories currently advertising. Choose your accessory.
- The accessory should be added now. You can add it to a specific room or set automation.
- The accessory should now be ready for use.

</details>
<BR>
