#!/bin/bash -e

ADK_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../"

# shellcheck source=Tools/
source "$ADK_ROOT/Tools/download.sh"

usage()
{
    echo "This script helps provision and install a given application on a connected device."
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "OPTIONS:"
    echo "-d  - [required] Device type. Possible value are: "
    echo "                 - nRF52"
    echo "                 - raspi"
    echo "                 - macOS"
    echo "-a  - [required] Path to the application to install"
    echo "-i  - [optional] install only, do not run the application. This argument is not supported for nRF52"
    echo "-n  - [optional] raspberry pi host name"
    echo "-N  - [optional] raspberry pi host IP address"
    echo "-p  - [optional] raspberry pi password"
    echo "-u  - [optional] raspberry pi user"
    echo "-t  - [optional] Transport type. Defaults to ble. Possible values are:"
    echo "                 - ip"
    echo "                 - ble"
    echo "                 - thread"
    echo "-l  - [optional] Install path on device. Default: '~'. This argument is not supported on macOS"
    echo "-k  - [optional] Create a HomeKit keystore file with a setup code of '111-22-333' and install on "
    echo "                 the connected device"
    echo "-e  - [optional] Specify the EUI to use for QR code generation.  If not provided a generic EUI will"
    echo "                 be used and the QR code created by AccessorySetupGenerator will be invalid"
    echo "-P  - [optional] Specify the Product data to use for QR code generation.  If not provided a generic"
    echo "                 product number will be provided and the QR code created will be invalid"
    echo "-s  - [optional] Select nRF52 device to use by index or serial number."
    echo "                 If only one device is available and this option is not specified, the device will be selected."
    echo "                 If there are more than one devices, this option is required."
    echo "                 Serial numbers are either printed on the boards or displayed if the selected device"
    echo "                 does not exist."
    echo "-c - [optional]  specifies the Category of the device.  Defaults to 5.  Categories are as follows:"
    echo "                     1  Other."
    echo "                     2  Bridges."
    echo "                     3  Fans."
    echo "                     4  Garage Door Openers."
    echo "                     5  Lighting."
    echo "                     6  Locks."
    echo "                     7  Outlets."
    echo "                     8  Switches."
    echo "                     9  Thermostats."
    echo "                     10  Sensors."
    echo "                     11  Security Systems."
    echo "                     12  Doors."
    echo "                     13  Windows."
    echo "                     14  Window Coverings."
    echo "                     15  Programmable Switches."
    echo "                     16  Range Extenders."
    echo "                     17  IP Cameras."
    echo "                     18  Video Doorbells."
    echo "                     19  Air Purifiers."
    echo "                     20  Heaters."
    echo "                     21  Air Conditioners."
    echo "                     22  Humidifiers."
    echo "                     23  Dehumidifiers."
    echo "                     24  Apple TV."
    echo "                     28  Sprinklers."
    echo "                     29  Faucets."
    echo "                     30  Shower Systems."
    echo "                     32  Remotes."
    echo "                     33  Wi-Fi Routers.";
    echo "-g  - [optional] Launch GDB server if the program aborts."
    exit 1
}

DEVICE=
APPLICATION=
HOSTNAME=
HOSTADDRESS=
USER=pi
PASSWORD=
EUI=
THREAD_TRANSPORT=0
BLE_TRANSPORT=0
IP_TRANSPORT=0
PRODUCT_DATA="03D8A775E3644573"
INSTALL_ONLY=
TRANSPORT_PROVIDED=0
INSTALL_PATH="~"
PROVISION_KEYSTORE=0
NORDIC_DEVICE_ID=-1
CATEGORY=5
GDB_ON_ABORT=0

ARGS="hd:a:n:N:p:it:l:ke:P:u:s:c:g"

while getopts ${ARGS} opt; do
    case ${opt} in
        d ) DEVICE=$OPTARG;;
        a ) APPLICATION=$OPTARG;;
        n ) HOSTNAME=$OPTARG;;
        N ) HOSTADDRESS=$OPTARG;;
        u ) USER=$OPTARG;;
        p ) PASSWORD=$OPTARG;;
        i ) INSTALL_ONLY=1;;
        t ) TRANSPORT_PROVIDED=1
            case ${OPTARG} in
                ble) BLE_TRANSPORT=1;;
                ip) IP_TRANSPORT=1;;
                thread) THREAD_TRANSPORT=1;;
                *) echo "Invalid transport value"
                   exit 1;;
            esac;;
        l ) INSTALL_PATH=$OPTARG;;
        k ) PROVISION_KEYSTORE=1;;
        e ) EUI=$OPTARG;;
        P ) PRODUCT_DATA=$OPTARG;;
        s ) NORDIC_DEVICE_ID=$OPTARG;;
        c ) CATEGORY=$OPTARG;;
        g ) GDB_ON_ABORT=1;;
        h ) usage;;
        \? ) usage;;
    esac
done

if (( TRANSPORT_PROVIDED == 0 )); then
   BLE_TRANSPORT=1
fi

if [[ -z "$DEVICE" || -z "$APPLICATION" ]]; then
    usage
fi

if [[ ! -f "$APPLICATION" ]]; then
    echo "File path $APPLICATION doesn't exist"
    exit 1
fi

PROVISIONING_FLAGS="--category ${CATEGORY} --setup-code 111-22-333 --product-data ""$PRODUCT_DATA"

FLASH_ADDR=000FA000
if ((BLE_TRANSPORT == 1)); then
    PROVISIONING_FLAGS="${PROVISIONING_FLAGS} --transport ble"
fi

if ((IP_TRANSPORT == 1)); then
    PROVISIONING_FLAGS="${PROVISIONING_FLAGS} --transport ip"
fi
if ((THREAD_TRANSPORT == 1)); then
    PROVISIONING_FLAGS="${PROVISIONING_FLAGS} --transport thread"
fi

TMP_DIR="$HOME/tmp/"
HOST=$(uname)

setup() {
    mkdir -p "$TMP_DIR"
}

makeTools() {
    # Make all the tools needed by provisioning script
    pushd "$ADK_ROOT" > /dev/null
    make tools
}

# shellcheck disable=SC2206
splitLines () {
    local IFS=$'\n'
    SPLIT_LINES=($1)
}

nordic() {
    # Verify that selected device exists
    jlink=$(find "$ADK_ROOT"/Output/"$HOST"-* -name jlink)
    device_list=$("$jlink" --list)
    i=0
    matched_device=-1

    splitLines "$device_list"
    serial_listing=()

    for line in "${SPLIT_LINES[@]}"
    do
        if [[ $i != 0 ]]; then
            read -r -a TOKENS <<< "$line"
            HEX_SERIAL=${TOKENS[1]}
            SERIAL=$((16#${TOKENS[1]}))
            declare -i j=$i-1
            serial_listing+=("[$j] $SERIAL")
            if [[ "$j" == "$NORDIC_DEVICE_ID" ]]; then
                matched_device=$HEX_SERIAL
            fi
            if [[ "$SERIAL" == "$NORDIC_DEVICE_ID" ]]; then
                matched_device=$HEX_SERIAL
            fi
            if [[ "$NORDIC_DEVICE_ID" == "-1" && "$j" == "0" && "${#SPLIT_LINES[@]}" == "2" ]]; then
                # User didn't select a device and there is only one device available.
                # Select the only device in such a case.
                matched_device=$HEX_SERIAL
            fi
        fi
        ((++i))
    done
    if [[ "$matched_device" != "-1" ]]; then
        NORDIC_DEVICE_ID=$matched_device
    else
        if [[ "${#serial_listing[@]}" == "0" ]]; then
            echo "No device is available."
            exit 1
        else
            echo "Invalid device selection. Pass -s option with one of the following [indices] or serial numbers:"
            for entry in "${serial_listing[@]}"
            do
                echo "$entry"
            done
        fi
        exit 1
    fi

    NORDIC_HOMEKIT_STORE_FILE="$TMP_DIR/HomeKitStore.hex"

    # Download Nordic SDK if not already
    NORDIC_SDK_DIR="$TMP_DIR/nRF5-SDK"
    NORDIC_SDK_ZIP="$TMP_DIR/nRF5-SDK.zip"
    NORDIC_SDK="$NORDIC_SDK_DIR"/components/softdevice/s140/hex/s140_nrf52_7.0.1_softdevice.hex
    NORDIC_SDK_SHA="ddd6a72bafddf8b122d4eaf214167250fc2352db89bb910039ed6312b2e546e3"

    # Check if nordic softdevice is the correct version
    if [[ -f "$NORDIC_SDK" ]]; then
        (echo "$NORDIC_SDK_SHA *${NORDIC_SDK}" | shasum -a 256 -cw) || rm -rf "${NORDIC_SDK_DIR}"
    fi

    if [[ ! -f "$NORDIC_SDK" ]]; then
        echo "Downloading nRF52 SDK..."
        rm -rf "$NORDIC_SDK_DIR"
        download "$NORDIC_SDK_ZIP" https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5-SDK-for-Thread/nRF5-SDK-for-Thread-and-Zigbee/nRF5SDKforThreadv41.zip
        unzip "$NORDIC_SDK_ZIP" -d "$NORDIC_SDK_DIR"
        rm -rf "$NORDIC_SDK_ZIP"
    fi

    if [[ "$PROVISION_KEYSTORE" -eq 1 ]]; then
        if [[ -n "$EUI" ]]; then
            PROVISIONING_FLAGS="${PROVISIONING_FLAGS}"" --eui ""$EUI"
        elif (( THREAD_TRANSPORT == 1 )); then
          # EUI Format: 3 bytes OUI (Organizationally Unique Identifier) 5 Bytes Unique identifier
          # NRF52 stores their Unique Identifier at 0x1000 0060 in 8 bytes.
          OUI="F4CE36"

          MEMREAD=()
          while IFS= read -r line; do
              MEMREAD+=( "$line" )
          done < <("$jlink" --memrd 0x10000060 --n 8 --select "$NORDIC_DEVICE_ID")

          read -r -a TOKENS <<< "${MEMREAD[1]}"
          RETRIEVED_EUI=${OUI}${TOKENS[2]}${TOKENS[1]:0:2}
          PROVISIONING_FLAGS="${PROVISIONING_FLAGS}"" --eui ""$RETRIEVED_EUI"
        fi

        # Generate a provisioning file
        # shellcheck disable=SC2086
        "$ADK_ROOT"/Tools/provision_nordic.sh --nfc ${PROVISIONING_FLAGS} "$NORDIC_HOMEKIT_STORE_FILE" $FLASH_ADDR

        # Erase flash before provisioning the file
        # Sometimes the erase doesn't seem to be erasing the device completely. Adding a little bit of delay
        # after erasing again seems to help. NOTE: Same issue exists with nrfjprog utility.
        echo "Erasing attached device"
        "$jlink" --select "$NORDIC_DEVICE_ID" --erase
        sleep 1
        "$jlink" --select "$NORDIC_DEVICE_ID" --erase
        sleep 1

        # Install the provisioning file on the device
        "$jlink" --select "$NORDIC_DEVICE_ID" --flash "$NORDIC_HOMEKIT_STORE_FILE"
    else
        # Run jlink --reset to put JLink into a known good state before jlink --run
        "$jlink" --select "$NORDIC_DEVICE_ID" --reset
    fi

    if [[ "$GDB_ON_ABORT" -eq 1 ]]; then
        gdboption=--gdbonabort
    fi

    realtime_option=
if ((THREAD_TRANSPORT == 1)); then
    realtime_option=--realtime
fi

    if [[ -n "$INSTALL_ONLY" ]]; then
        # Flash softdevice and application on the connected nordic board
        "$jlink" \
            --select "$NORDIC_DEVICE_ID" \
            --flash $gdboption $realtime_option \
            "$NORDIC_SDK_DIR"/components/softdevice/s140/hex/s140_nrf52_7.0.1_softdevice.hex  \
            "$APPLICATION"
    else
        # Flash and run softdevice and application on the connected nordic board
        "$jlink" \
            --select "$NORDIC_DEVICE_ID" \
            --run $gdboption $realtime_option \
            "$NORDIC_SDK_DIR"/components/softdevice/s140/hex/s140_nrf52_7.0.1_softdevice.hex  \
            "$APPLICATION"

    fi
}

raspi() {
    if [[ -z "$HOSTADDRESS" ]]; then
        HOSTADDRESS="$HOSTNAME.local"
    fi

    # Copy over the application requested.
    expect <<EOF
    set timeout -1
    spawn scp -o ConnectTimeout=30 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -r "$APPLICATION" ${USER}@${HOSTADDRESS}:${INSTALL_PATH}/
    expect {
        "password:"  { send "${PASSWORD}\n"; exp_continue }
        eof
    }
    lassign [wait] pid spawnID osError value
    exit \$value
EOF

    # Generate a HomeKit keystore file and install on the device
    if [[ "$PROVISION_KEYSTORE" -eq 1 ]]; then
        # Clear any existing HomeKit keystore and create install path if necessary
        expect <<EOF
        set timeout -1
        spawn ssh -o ConnectTimeout=30 -o ServerAliveInterval=10000 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null ${USER}@${HOSTADDRESS}
        expect {
            "(yes/no)? " { send "yes\n"; exp_continue }
            "password:"  { send "${PASSWORD}\n"; exp_continue }
            "${USER}@${HOSTNAME}"
        }
        send "sudo rm -rf ${INSTALL_PATH}/.HomeKitStore\n"
        expect "${USER}@${HOSTNAME}"
        send "mkdir -p ${INSTALL_PATH}\n"
        expect "${USER}@${HOSTNAME}"
        send "exit\n"
        expect eof
        lassign [wait] pid spawnID osError value
        exit \$value
EOF
        EXTRA_ARGS=

        # Generate a provisioning file
        expect <<EOF
        spawn ${ADK_ROOT}/Tools/provision_posix.sh --category ${CATEGORY} --product-data ${PRODUCT_DATA} --setup-code 111-22-333 ${EXTRA_ARGS} ${USER}@${HOSTADDRESS}:${INSTALL_PATH}/.HomeKitStore
        expect {
            "password:"  { send "${PASSWORD}\n"; exp_continue }
            eof
        }
        lassign [wait] pid spawnID osError value
        exit \$value
EOF
    fi

    if [[ -z "$INSTALL_ONLY" ]]; then
        # Run it
        expect <<EOF
        set timeout -1
        spawn ssh -o ConnectTimeout=30 -o ServerAliveInterval=10000 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null ${USER}@${HOSTADDRESS}
        expect {
            "password:"  { send "${PASSWORD}\n"; exp_continue }
            "${USER}@${HOSTNAME}"
        }
        send "cd ${INSTALL_PATH}\n"
        expect "${USER}@${HOSTNAME}"
        send "sudo ./$(basename "$APPLICATION")\n"
        expect eof
        lassign [wait] pid spawnID osError value
        exit \$value
EOF
    fi
}

macOS() {
    # Install the HomeKit keystore at the same location as the path to the application
    INSTALL_PATH=$(dirname "$APPLICATION")

    # Generate a HomeKit keystore file and install on the device
    if [[ "$PROVISION_KEYSTORE" -eq 1 ]]; then
        # Clear any existing HomeKit keystore and create install path if necessary
        rm -rf "${INSTALL_PATH}/.HomeKitStore"
        mkdir -p "${INSTALL_PATH}"

        EXTRA_ARGS=

        # Generate a provisioning file
        # shellcheck disable=SC2086 # Double quote to prevent globbing
        "${ADK_ROOT}"/Tools/provision_posix.sh --category "${CATEGORY}" --product-data "${PRODUCT_DATA}" --setup-code 111-22-333 ${EXTRA_ARGS} "${INSTALL_PATH}/.HomeKitStore"
    fi

    if [[ -z "$INSTALL_ONLY" ]]; then
        # Run it
        "./$APPLICATION"
    fi
}

LC_DEVICE=$(echo "$DEVICE" | awk '{print tolower($0)}')

case "$LC_DEVICE" in
    "nrf52")
        setup
        makeTools
        nordic
        ;;
    "raspi")
        makeTools
        raspi
        ;;
    "macos")
        makeTools
        macOS
        ;;
    *)
        echo "Device $DEVICE is not supported."
        exit 1
        ;;
esac
