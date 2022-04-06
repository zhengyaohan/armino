Provisioning Tools
==================

- [AccessorySetupGenerator](#accessorysetupgenerator)
- [provision_posix.sh](#provision-posix-sh)
- [install.sh](#install-sh)

## AccessorySetupGenerator

The *ADK/Tools* directory contains the *AccessorySetupGenerator* tool for macOS and Linux. This tool can be used by
provisioning mechanisms (e.g., the script in `ADK/Tools/provision_posix.sh`) to generate information for the provisioning
of a HomeKit accessory, such as a setup code, a corresponding SRP salt and verifier, and a setup ID. The setup code is
used by an iOS device for setting up an encrypted link with the accessory during HomeKit pairing. The setup ID is used
to identify the accessory to which a scanned label belongs. Start the tool to see the options that it supports. The
output of the generator is sent to stdout, whereas stderr is used for error conditions.

### Compile

```sh
make tools
```

### Run
From the Mac, you can create the provisioning data, e.g., for LightBulb:

```sh
./Output/Darwin-x86_64-apple-darwin$(uname -r)/Tool/Tools/AccessorySetupGenerator.OpenSSL \
    --ip \
    --category 5 \
    --product-data <product_data>
```

This produces the following output in stdout:

```sh
1
358-16-884
C3298DA07B626F544394863A54D5B6F2
5CF40BEE1DAD2B213CCC7AC671174874EFB0F10E82F5323B818E5489ECC89F17B19C18829098463304B92C1C20C3084C9E0B46998AF8CA7C95A7F40FFA81E80CACA2314A76CD3830D6731B05612E2C448C0503B0F272164676FA73009191BD32E9EE23BCFFE390798B405C8CE7E63A5978AB2E76896C90B04DC8FB9875281566EBA5734E2C5ADFBE50BB0CC86C1D9046271004FCE7C631D0E2733617669BDF0E9226EFFEBEA4BCC5B3CA905FB7099C3DE2552A55069C8EED209F36FC9F9DA1AD3BEE6FA9B1672B43E9A9693963FA6127B15AAD208B6D23FB3C2E1A614EA3342B833EDE7BD24AFF58A3A1F5DE31863DD0FA39C32553FE19366E74BC0B420E0F0CD52A80B92ABD7A7B43B5185DB556E1F611E21D372A3EC47B93B2B7678AB056B60BF4BED28EA6C22D8B1E76C9885AD996CA7B9E122ABE069DD2ACB3B80E0BC9DB0B738F8541084633D483420543F2D89E382B498B6F41DC3964057357AE009C858477A9544C8D4112D238B5B1D89708D5E98F952F6E09DEBB597FBBE4CBEE0023
GY2B
X-HM://0E66IRD84GY2B0YJH2CJFPA4DMRWGSG
```

The Provision script writes the generated values into files in folder `.KeyValueStore`:
- File .HomeKitStore/40.10 contains SRP salt and verifier
- File .HomeKitStore/40.11 contains setup ID

## Note For the Accessory Vendors

For a licensed accessory, a random setup code and related data **must** be generated for every individual accessory
according to the rules described in the HomeKit Accessory Protocol Specification, and as summarized here:
-  Generate a random setup code in format *XXX-XX-XXX* with *X* being a digit from *0-9*. The code must be generated
using a cryptographically secure random number generator.
    - Codes that only consist of a repeating digit are not allowed.
    - *123-45-678* and *876-54-321* are not allowed.
- Generate a random SRP salt (16 random bytes) using a cryptographically secure random number generator.
- Derive the corresponding SRP verifier from the setup code and SRP salt.
- Generate a random setup ID in format *XXXX* with *X* being a digit from *0-9* or a character from *A-Z*. The ID must be
generated using a cryptographically secure random number generator.
    - Lowercase characters are not allowed.
- Derive the setup payload from the setup code and setup ID.
    - For manual validation of this task, the HomeKit Accessory Simulator may be used by opening it, then pressing Cmd-2.
- Deploy the SRP salt, the SRP verifier and the setup ID to the accessory.
    - If the accessory supports programmable NFC - and only then - also deploy the setup code.
    - Depending on how these items are deployed to an accessory, `HAPPlatformAccessorySetup.c` must be adapted accordingly.
- Create a QR code sticker that contains the setup payload and setup code. The sticker will be used by users every time
they want to pair the accessory to an iOS device.
    - Implement the QR code as specified in the HomeKit Identity Guidelines.

If the accessory has a display that supports displaying a setup code or QR code, only the setup ID has to be generated
and deployed in the manufacturing process. The other steps involving setup code, SRP salt, SRP verifier, setup payload,
and the QR code label may be ignored.

The provisioning script `ADK/Tools/provision_posix.sh` serves as a reference implementation of the provisioning
mechanism. With this tool, a target accessory's key-value store is provisioned with data that is compatible with the
provided implementation of PAL module HAPPlatformAccessorySetup. If the PAL implementation is modified, e.g., in order
to use some other storage mechanism for storing the provisioning information, then this tool must be adapted accordingly.

The Provision tool can be used for hardware authentication (Apple Authentication Coprocessor) as well as software
authentication. Its usage is printed if the script is run without any arguments.

``` Caution::
    Warning: When you provision a setup or QR code, be aware that for security reasons it is not possible to reconstruct
    it from data stored on the accessory.
```

## provision_posix.sh

This is a primary tools to provision a *Posix* based HomeKit accessory. This tool is used to generate a
HomeKitStore with the required provisioning information to allow pairing. In the sample POSIX PAL, this HomeKitStore
is a directory containing several files that contains information such as SRP salt and verifier that is required for
pairing a HomeKit ADK accessory with an iOS device along with tokens required for Software Authentication if used.
Example usage:

```sh
Tools/provision_posix.sh
 --category <category> \
 --product-data <product_data> \
 --setup-code <setup_code> \
 <destination>
```

### Provision a Software Authentication Token
Please refer to Software Authentication section of [Accessory Authentication](accessory_authentication.md) for more
details.

## install.sh

The tool `install.sh` is a wrapper around `provision_posix.sh` tool and is only provided for convenience. This tool
**doesn't** support all the functionality of the `provision_posix.sh` tool. A typical use of this tool is to provision
an ADK application, copy the ADK application binary on a Raspberry Pi and to run the ADK application in one step
instead of separate individual steps. Example usage:

```sh
./Tools/install.sh \
    -d raspi \
    -t ip \
    -a Output/Raspi-armv6k-unknown-linux-gnueabihf/Debug/IP/Applications/Lightbulb.OpenSSL \
    -n <host_name> \
    -p <password> \
    -u <user> \
    -k
```

- `-k` argument is to create a key-value store with a default setup code of `111-22-333`. Setting this option
clears up the previous key-value store store. So, if you don't want to have to pair the application again with Home
app on your iOS device the next time you run the application please make sure to *not include* the argument `-k`
when running the script.

``` Note::
    Please run ``Tools/install.sh -h`` for more information.
```
