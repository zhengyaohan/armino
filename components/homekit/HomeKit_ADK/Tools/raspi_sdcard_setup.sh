#!/bin/bash -e

set -eu -o pipefail
SOURCE="$0"
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADK_ROOT="$DIR/../"

# shellcheck source=Tools/
source "$ADK_ROOT/Tools/download.sh"

# Set environment variables.
export ROOT="${DIR}"
if [ -z "${CACHE:-}" ]; then
    export CACHE="${ROOT}/.cache"
fi
export SSH_TMP="${ROOT}/.ssh_temp"

export LC_ALL=en_GB.UTF-8
export LANG=en_GB.UTF-8

IDENTITY_FILE="${SSH_TMP}/raspisetup"
SSH_OPTS=(-i "$IDENTITY_FILE" -o 'StrictHostKeyChecking=no' -o 'UserKnownHostsFile=/dev/null' -o 'ConnectTimeout=30' -o 'ServerAliveInterval=10000')

########################################################################################################################

usage()
{
    echo "This script is used to create an SD card SD card first with a Raspberry Pi OS"
    echo "image along with additional dependencies required by the ADK"
    echo ""
    echo "Usage: $0"
    echo ""
    exit 1
}

OPTS="h"
while getopts "${OPTS}" opt; do
    case ${opt} in
        h ) usage;;
        \? ) usage;;
        * ) usage;;
    esac
done

################################################################################
# Sleep countdown function
################################################################################
# $1: How long to sleep for
do_sleep() {
    COUNT=${1}
    sp="/-\|"
    echo -n '      '
    while [[ ${COUNT} -gt -1 ]]; do
        printf '\r%.1s %3d' "$sp" "$COUNT"
        sp=${sp#?}${sp%???}
        COUNT=$((COUNT - 1))
        sleep 1
    done
    printf "\r     \n"
}

################################################################################
# Waits for raspi to come back after reboot.
################################################################################
# $1: hostname
# $2: max retries
wait_for_reboot() {
    # Sleep for a few seconds to allow reboot to begin, else we risk getting a
    # valid ping before raspi shutdown is completed.
    do_sleep 5

    echo "Waiting for raspi to reboot..."
    COUNT=${2}
    sp="/-\|"
    echo -n '      '
    while ! ping -c 1 "$1" &> /dev/null; do
        printf '\r%.1s %3d' "$sp" "$COUNT"
        sp=${sp#?}${sp%???}
        COUNT=$((COUNT - 1))
        if [[ ${COUNT} -lt 0 ]]; then
            printf "\r     \n"
            echo "Failed to reconnect to raspberry pie."
            exit 1
        fi
        sleep 1
    done
    printf "\r     \n"
}

# Runs a command on the Raspi.
# $1: Hostname.
# $2: Password.
deploy_ssh_key() {
    mkdir -p "${SSH_TMP}"
    ssh-keygen -t rsa -N '' -f "${IDENTITY_FILE}" -C raspi@setup.local

    expect <<EOF
    set timeout -1
    spawn ssh-copy-id ${SSH_OPTS[@]} -f pi@${1}.local
    expect "password:"
    send "${2}\n"
    expect eof
    lassign [wait] pid spawnID osError value
    exit \$value
EOF
}

# $1: File name / folder name.
# $2: Hostname of Raspberry Pi.
# $3: Password for user account `pi`.
deploy() {
    local RASPI="${2}"
    expect <<EOF
    set timeout -1
    spawn scp ${SSH_OPTS[@]} -r {${1}} pi@${RASPI}.local:~
    expect eof
    lassign [wait] pid spawnID osError value
    exit \$value
EOF
}

# Runs a command on the Raspi.
# $1: Hostname.
# $2: Password.
# $3: Command.
RaspiExec() {
    expect <<EOF
    set timeout -1
    spawn ssh ${SSH_OPTS[@]} "pi@${1}" {${3}}
    expect eof
    lassign [wait] pid spawnID osError value
    exit \$value
EOF
}

# $1: Hostname of Raspberry Pi.
# $2: Password for user account `pi`.
reboot_pi() {
    local RASPI="${1}"
    expect <<EOF
    set timeout -1
    spawn ssh ${SSH_OPTS[@]} pi@${RASPI}.local
    expect "pi@${RASPI}"
    send "sudo reboot\n"
    expect eof
EOF
    wait_for_reboot "${RASPI}.local" 60
}

setup_sd_card() {
    local CONFIRM
    local SD_CARD_DISK

    # Find disk name.
    echo "================================================================================"
    echo "Please power off Raspberry Pi and remove SD card, and insert card into host."
    read -r -s -p "Press [return] to continue."
    echo
    echo "================================================================================"
    while [ -z "${SD_CARD_DISK}" ]; do
        diskutil list
        echo "================================================================================"
        echo "Please type in Raspberry Pi SD card device name, or press enter to refresh disk list."
        read -r -p "/dev/disk" SD_CARD_DISK
    done
    echo "================================================================================"
    read -r -p "Erase this disk: \`/dev/disk$SD_CARD_DISK\`? [y/N]" CONFIRM
    echo
    [ "$CONFIRM" == "y" ] || [ "$CONFIRM" == "Y" ]
    echo "================================================================================"

    # Erase disk image.
    set -x
    diskutil unmountDisk "/dev/disk$SD_CARD_DISK"
    sudo diskutil eraseDisk FAT32 RASPBIAN MBRFormat "/dev/disk$SD_CARD_DISK"
    diskutil unmountDisk "/dev/disk$SD_CARD_DISK"
    sleep 5

    # Get disk image.
    # https://www.raspberrypi.org/downloads/raspbian/
    local URL_BASE="http://director.downloads.raspberrypi.org/raspios_lite_armhf/images"
    local URL_SUB="raspios_lite_armhf-2021-01-12"
    local VERSION="2021-01-11-raspios-buster-armhf"
    echo "Setting up SD card: /dev/disk$SD_CARD_DISK"
    download "${CACHE}/${VERSION}-lite.zip" "${URL_BASE}/${URL_SUB}/${VERSION}-lite.zip" d49d6fab1b8e533f7efc40416e98ec16019b9c034bc89c59b83d0921c2aefeef
    unzip "${CACHE}/${VERSION}-lite.zip" -d .tmp

    # Deploy disk image.
    echo "(Check \`dd\` progress with ^T)"
    time sudo dd bs=1m if=".tmp/${VERSION}-lite.img" of="/dev/rdisk$SD_CARD_DISK"
    rm ".tmp/${VERSION}-lite.img"
    sleep 5
    set +x

    # Enable SSH.
    touch /Volumes/boot/ssh

    # Disable expand filesystem on boot.
    perl -i -p0e 's/ init=\/usr\/lib\/raspi-config\/init_resize\.sh//gs' "/Volumes/boot/cmdline.txt"

    sudo diskutil eject "/dev/rdisk$SD_CARD_DISK"
    sleep 5
}

configure_timezone() {
    PI_TIMEZONE="${NEW_PI_TIMEZONE:-}"

    # If the timezone was not provided to the script, fetch the host's timezone
    if [ "${PI_TIMEZONE}" == "" ]; then
        unameOut="$(uname -s)"
        case "${unameOut}" in
            Linux*)     PI_TIMEZONE=$(cat /etc/timezone);;
            Darwin*)    PI_TIMEZONE=$(readlink /etc/localtime | sed -e "s/^\/var\/db\/timezone\/zoneinfo\///");;
            *)          return;;
        esac
    fi

    # Send timezone to raspi
    echo "Setting the timezone of the raspberry pi to '$PI_TIMEZONE'"
    expect <<EOF
        set timeout -1
        spawn ssh ${sopts[@]} pi@${host}
        expect "pi@${host}'s password:"
        send "${pw}\n"
        expect "pi@${hostname}"
        send "sudo timedatectl set-timezone ${PI_TIMEZONE} \n"
        expect "pi@${hostname}"
        send "exit\n"
        expect eof
EOF

}

resize_prebuilt_sdcard_to_full() {
    echo "================================================================================"
    echo
    echo "CHECKPOINT 1: Raspberry Pi prebuilt image setup complete"
    echo
    echo "To restart from this point, run the following command:"
    echo "   \$ SETUP_RESIZE_PREBUILT_IMAGE=1 $SOURCE"
    echo "================================================================================"

    echo "================================================================================"
    echo "1. Put SD card into Raspberry Pi."
    echo "2. Connect Raspberry Pi to host via Ethernet (raspi3 & 4) / USB (raspi zero)."
    echo "3. Power Raspberry Pi."
    read -r -s -p "Press [return] to continue."
    echo
    echo "================================================================================"

    local hostname="raspberrypi"
    local host="${hostname}.local"
    local pw="raspberry"
    local sopts=(-o 'StrictHostKeyChecking=no' -o 'UserKnownHostsFile=/dev/null' -o 'ConnectTimeout=30' -o 'ServerAliveInterval=10000')

    wait_for_reboot "${host}" 60

    echo "Unblocking WiFi ..."

    expect <<EOF
        set timeout -1
        spawn ssh ${sopts[@]} pi@${host}
        expect "pi@${host}'s password:"
        send "${pw}\n"
        expect "pi@${hostname}"
        send "sudo rfkill unblock wifi\n"
        expect "pi@${hostname}"
        send "exit\n"
        expect eof
EOF
    echo

    configure_timezone

    echo "Resizing partition to full size ..."

    expect <<EOF
        set timeout -1
        spawn ssh ${sopts[@]} pi@${host}
        expect "pi@${host}'s password:"
        send "${pw}\n"
        expect "pi@${hostname}"
        send "sudo parted /dev/mmcblk0 resizepart 2 100%\n"
        expect "pi@${hostname}"
        send "sudo resize2fs /dev/mmcblk0p2\n"
        expect "pi@${hostname}"
        send "sudo reboot\n"
        expect eof
EOF
    echo
    echo "Done resizing. SD card is ready to use."
}

# $1: current hostname of Raspberry Pi.
# $2: current password of Raspberry Pi.
# $3: total size to be used on disk in MB.
setup_raspberry_pi() {
    local RASPI="${1}"
    local PW="${2}"
    local SIZE_MEGABYTES="${3}"

    # Expand file system.
    # See /usr/bin/raspi-config.
    echo "Resizing file system to use up to: ${SIZE_MEGABYTES} MB"
    rootPart="$(RaspiExec "${RASPI}.local" "${PW}" \
        "mount | sed -n 's|^/dev/\(.*\) on / .*|\1|p'" | tail -n 1)"
    rootPart="${rootPart/$'\r'/}"
    partNum="${rootPart#mmcblk0p}"
    if [ "${partNum}" == "${rootPart}" ]; then
        echo "Unexpected error while partitioning (1): ${partNum} == ${rootPart}"
        false
    fi
    if [ "${partNum}" != "2" ]; then
        echo "Unexpected error while partitioning (2): ${partNum} != 2"
        false
    fi
    lastPartNum="$(RaspiExec "${RASPI}.local" "${PW}" \
        "sudo parted /dev/mmcblk0 -ms unit s p | tail -n 1 | cut -f 1 -d:" | tail -n 1)"
    lastPartNum="${lastPartNum/$'\r'/}"
    if [ "${lastPartNum}" != "${partNum}" ]; then
        echo "Unexpected error while partitioning (3): ${lastPartNum} != ${partNum}"
        false
    fi

    cmd="sudo parted /dev/mmcblk0 -ms unit s p | grep \"^/dev/mmcblk0\" | cut -f 4 -d: | sed 's/[^0-9]//g'"
    sectorSize="$(RaspiExec "${RASPI}.local" "${PW}" "${cmd}" | tail -n 1)"
    sectorSize="${sectorSize/$'\r'/}"
    # all sizes in sectors:
    cmd="sudo parted /dev/mmcblk0 -ms unit s p | grep \"^${partNum}\" | cut -f 2 -d: | sed 's/[^0-9]//g'"
    partStart="$(RaspiExec "${RASPI}.local" "${PW}" "${cmd}" | tail -n 1)"
    partStart="${partStart/$'\r'/}"
    diskSize=$(((SIZE_MEGABYTES * 1000000) / 512)) # modulo size is wasted
    echo "Size to be used: ${diskSize}s"
    partSize=$((diskSize - partStart))
    echo "Data partition size: ${partSize}s"
    if (( partSize < 0 )); then
        echo "Requested disk size too small: ${SIZE} MB. Must be greater than $(( partStart * sectorSize ))"
        false
    fi
    RaspiExec "${RASPI}.local" "${PW}" \
        "sudo fdisk /dev/mmcblk0 <<<\$'p\nd\n${partNum}\nn\np\n${partNum}\n${partStart}\n+${partSize}\np\nw\n'" || true
    reboot_pi "${RASPI}" "${PW}"
    RaspiExec "${RASPI}.local" "${PW}" "sudo resize2fs /dev/${rootPart}"
    echo "File system resized."
    reboot_pi "${RASPI}" "${PW}"

    # set date
    currentTime="$(date -u)"
    echo "${currentTime}"

    expect <<EOF
    set timeout -1
    spawn ssh ${SSH_OPTS[@]} "pi@${RASPI}.local"
    expect "pi@${RASPI}"

    send "sudo date -u -s '${currentTime}'\n"
    expect "pi@${RASPI}"

    send "sudo raspi-config\n"

    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\r"
    expect "Would you like the camera interface to be enabled?"
    send "\x1B\[D"
    send "\r"
    expect "The camera interface is enabled"
    send "\r"

    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Would you like the SPI interface to be enabled?"
    send "\x1B\[D"
    send "\r"
    expect "The SPI interface is enabled"
    send "\r"

    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Would you like the ARM I2C interface to be enabled?"
    send "\x1B\[D"
    send "\r"
    expect "The ARM I2C interface is enabled"
    send "\r"

    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Would you like a login shell to be accessible"
    send "\r"
    expect "The serial login shell is enabled"
    send "\r"

    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\x1B\[B"
    send "\r"
    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\x1B\[B"
    send "\r"
    expect "How much memory (MB) should the GPU have?  e.g. 16/32/64/128/256"
    send "\010\010\010"
    send "256"
    send "\r"

    expect "Raspberry Pi Software Configuration Tool (raspi-config)"
    send "\t"
    send "\t"
    send "\r"
    expect "Would you like to reboot now?"
    send "\r"
    expect eof
EOF
    wait_for_reboot "${RASPI}.local" 60

    expect <<EOF
    set timeout -1
    spawn ssh ${SSH_OPTS[@]} "pi@${RASPI}.local"
    expect "pi@${RASPI}"

    send "echo -en "
    send "'--- /etc/modules\\\n'"
    send "'+++ /etc/modules\\\n'"
    send "'@@ -4,3 +4,4 @@\\\n'"
    send "' # at boot time, one per line. Lines beginning with \"#\" are ignored.\\\n'"
    send "' \\\n'"
    send "' i2c-dev\\\n'"
    send "'+bcm2835-v4l2\\\n'"
    send " | sudo patch -p0 -d /\n"
    expect "pi@${RASPI}"
    send "sudo reboot\n"
    expect eof
EOF
    wait_for_reboot "${RASPI}.local" 60
}

# $1: current hostname of Raspberry Pi.
# $2: new hostname of Raspberry Pi.
# $3: new password of Raspberry Pi.
change_hostname_and_password() {
    local CURRENT_HOSTNAME="${1}"
    local NEW_HOSTNAME="${2}"
    local NEW_PASSWORD="${3}"
    export LC_ALL=en_GB.UTF-8
    export LANG=en_GB.UTF-8
    expect <<EOF
    set timeout -1
    spawn ssh ${SSH_OPTS[@]} "pi@${CURRENT_HOSTNAME}.local"
    expect {
        "pi@${CURRENT_HOSTNAME}.local's password:" {
            send "raspberry\n"
            exp_continue
        }
        "pi@${CURRENT_HOSTNAME}"
    }
    send "sudo su\n"
    expect "root@${CURRENT_HOSTNAME}"
    send "sed -i s/${CURRENT_HOSTNAME}/${NEW_HOSTNAME}/ /etc/hostname\n"
    expect "root@${CURRENT_HOSTNAME}"
    send "sed -i \"s/127.0.1.1.*${CURRENT_HOSTNAME}/127.0.1.1\\\t${NEW_HOSTNAME}/g\" /etc/hosts\n"
    expect "root@${CURRENT_HOSTNAME}"
    send "rm /etc/ssh/ssh_host_*\n"
    expect "root@${CURRENT_HOSTNAME}"
    send "dpkg-reconfigure openssh-server\n"
    expect "root@${CURRENT_HOSTNAME}"
    send "passwd pi\n"
    expect "New password:"
    send "${NEW_PASSWORD}\n"
    expect "Retype new password:"
    send "${NEW_PASSWORD}\n"
    expect "passwd: password updated successfully"
    expect "root@${CURRENT_HOSTNAME}"
    send "reboot\n"
    expect eof
EOF
    wait_for_reboot "${NEW_HOSTNAME}.local" 60
}

create_image() {
    local IMG_NAME="${1}"
    local DISK_SIZE="${2}"
    local CONFIRM
    local SD_CARD_DISK

    # Find disk name.
    echo "================================================================================"
    echo "Please power off Raspberry Pi and remove SD card."
    read -r -s -p "Press [return] to continue."
    echo
    echo "================================================================================"
    diskutil list
    echo "================================================================================"
    echo "Please insert Raspberry Pi SD card into host."
    read -r -s -p "Press [return] to continue."
    echo
    echo "================================================================================"
    sleep 10
    diskutil list
    echo "================================================================================"
    read -r -p "Please type Raspberry Pi SD card device name: /dev/disk" SD_CARD_DISK
    read -r -p "Create image from this disk: \`/dev/disk$SD_CARD_DISK\`? [y/N]" CONFIRM
    echo
    [ "$CONFIRM" == "y" ] || [ "$CONFIRM" == "Y" ]
    echo "================================================================================"

    set -x
    diskutil unmountDisk "/dev/disk$SD_CARD_DISK"
    sleep 5

    # Create disk image file.
    echo "(Check \`dd\` progress with ^T)"
    sudo dd bs=1m count="${DISK_SIZE}" if="/dev/rdisk$SD_CARD_DISK" of="${IMG_NAME}"
    sleep 5
    set +x

    sudo diskutil eject "/dev/rdisk$SD_CARD_DISK"
    sleep 5
}

########################################################################################################################

# Prepare temporary folder.
cd "${ROOT}"
rm -rf .tmp
mkdir -p .tmp

if [ "${SETUP_SKIP_SD:-0}" != "1" ]; then
    setup_sd_card

    echo "================================================================================"
    echo
    echo "CHECKPOINT 1: SD card loaded with base image"
    echo
    echo "To restart from this point, run the following command:"
    echo "   \$ SETUP_SKIP_SD=1 $SOURCE"
    echo "================================================================================"

    echo "================================================================================"
    echo "1. Put SD card into Raspberry Pi."
    echo "2. Connect Raspberry Pi to host via Ethernet (raspi3 & 4) / USB (raspi zero)."
    echo "3. Power Raspberry Pi."
    read -r -s -p "Press [return] to continue."
    echo
    echo "================================================================================"
    wait_for_reboot "raspberrypi.local" 60
fi

PI_HOSTNAME="raspberrypi"
PI_PASSWORD="${PASSWORD:-raspberry}"
PI_COUNTRY="${PI_COUNTRY:-}"
SD_SIZE_MB="4900"

if [ "${SETUP_SKIP_BASE_SETUP:-0}" != "1" ]; then
    if [ -z "${SETUP_HOSTNAME+x}" ]; then
        R="$(head /dev/urandom | LC_ALL=C tr -cd '0-9' | head -c 4)"
        SETUP_HOSTNAME="raspi-${R}"
    fi

    echo "SETUP_HOSTNAME: ${SETUP_HOSTNAME}"
    SETUP_PASSWORD="${SETUP_HOSTNAME}"

    deploy_ssh_key "${PI_HOSTNAME}" "${PI_PASSWORD}"
    change_hostname_and_password "${PI_HOSTNAME}" "${SETUP_HOSTNAME}" "${SETUP_PASSWORD}"
    setup_raspberry_pi "${SETUP_HOSTNAME}" "${SETUP_PASSWORD}" "${SD_SIZE_MB}"

    echo "================================================================================"
    echo
    echo "CHECKPOINT 2: Raspberry Pi base setup complete"
    echo
    echo "To restart from this point, run the following command:"
    echo "   \$ SETUP_SKIP_SD=1 SETUP_SKIP_BASE_SETUP=1 SETUP_HOSTNAME=$SETUP_HOSTNAME SETUP_PASSWORD=$SETUP_PASSWORD $SOURCE"
    echo "================================================================================"
else
    SETUP_HOSTNAME="${SETUP_HOSTNAME:-${PI_HOSTNAME}}"
    SETUP_PASSWORD="${SETUP_PASSWORD:-${PI_PASSWORD}}"
fi

# Install packages.
if [ "${SETUP_SKIP_PACKAGES:-0}" != "1" ]; then
    echo "================================================================================"
    echo "Please connect the Raspberry Pi to the internet via wifi or by sharing your"
    echo "internet connection."
    echo "On Mac: System Preferences > Sharing > Internet Sharing"
    echo
    read -r -s -p "Press [return] to continue."
    echo

    expect <<EOF
        set timeout -1
        spawn ssh ${SSH_OPTS[@]} pi@${SETUP_HOSTNAME}.local
        expect "pi@${SETUP_HOSTNAME}"
        send "sudo reboot\n"
        expect eof
EOF
    wait_for_reboot "${SETUP_HOSTNAME}.local" 60
    PACKAGE_FILE_NAME="install_packages.sh"
    INSTALL_PACKAGE_FILE="${ROOT}/${PACKAGE_FILE_NAME}"
    deploy "${INSTALL_PACKAGE_FILE}" "${SETUP_HOSTNAME}" "${SETUP_PASSWORD}"

    VERSION_MDNS="1310.80.1" # https://opensource.apple.com
    download \
        "${CACHE}/mDNSResponder-${VERSION_MDNS}.tar.gz" \
        "https://opensource.apple.com/tarballs/mDNSResponder/mDNSResponder-${VERSION_MDNS}.tar.gz" \
        097662447e1535573484697861d9f50eceaf2c52ec2742e451ee6ffe9bbf3e75
    deploy "${CACHE}/mDNSResponder-${VERSION_MDNS}.tar.gz" "${SETUP_HOSTNAME}" "${SETUP_PASSWORD}"
    deploy \
        "${ADK_ROOT}/Build/Docker/Patches/mDNSResponder/0001-Fix-mdns-v1310.80.1-compilation.patch" \
        "${SETUP_HOSTNAME}" "${SETUP_PASSWORD}"

    VERSION_LIBNFC="1.7.1" # https://github.com/nfc-tools/libnfc/releases
    download \
        "${CACHE}/libnfc-${VERSION_LIBNFC}.tar.bz2" \
        "https://github.com/nfc-tools/libnfc/releases/download/libnfc-${VERSION_LIBNFC}/libnfc-${VERSION_LIBNFC}.tar.bz2" \
        945e74d8e27683f9b8a6f6e529557b305d120df347a960a6a7ead6cb388f4072
    deploy "${CACHE}/libnfc-${VERSION_LIBNFC}.tar.bz2" "${SETUP_HOSTNAME}" "${SETUP_PASSWORD}"

    RaspiExec "${SETUP_HOSTNAME}.local" "x" "VERSION_MDNS=\"${VERSION_MDNS}\" VERSION_LIBNFC=\"${VERSION_LIBNFC}\" bash ./install_packages.sh"
    RaspiExec "${SETUP_HOSTNAME}.local" "x" "rm ${PACKAGE_FILE_NAME}"

    "${ROOT}/raspi_bluez_install.py" -n "${SETUP_HOSTNAME}" -p "${SETUP_PASSWORD}"
    RaspiExec "${SETUP_HOSTNAME}.local" "x" "rm -fr bluez-5.50.tar.xz patch_raspi_bluez.py"

    echo "================================================================================"
    echo
    echo "CHECKPOINT 3: All dependencies installed."
    echo
    echo "To restart from this point, run the following command:"
    echo "   \$ SETUP_SKIP_SD=1 SETUP_SKIP_BASE_SETUP=1 SETUP_SKIP_PACKAGES=1 SETUP_HOSTNAME=$SETUP_HOSTNAME SETUP_PASSWORD=$SETUP_PASSWORD $SOURCE"
    echo "================================================================================"

fi
# END Install packages

if [ "${SETUP_SKIP_WLAN_CONFIG:-0}" != "1" ]; then

    # Collect regulatory domain.
    while [ -z "${PI_COUNTRY}" ]; do
        echo "Please enter the country in which the Raspberry Pi will be used ([return] to show list of country codes)."
        read -r -p "Country code: " PI_COUNTRY
        if [ -z "${PI_COUNTRY}" ]; then
            # Print all country codes.
            i=0
            colWidth=43
            colsPerLine=$(( $(tput cols) / (colWidth + 3) ))
            if (( !colsPerLine )); then
                colsPerLine=1
            fi
            numLines=$(grep -c -E '^[A-Z]{2}\t' "/usr/share/zoneinfo/iso3166.tab")
            linesPerCol=$(( (numLines + (colsPerLine - 1) ) / colsPerLine ))
            paddingLines=$(( linesPerCol * colsPerLine - numLines ))

            countries="$(grep -E '^[A-Z]{2}\t' "/usr/share/zoneinfo/iso3166.tab")"$'\n'
            for (( i=0; i<paddingLines; i++ )); do
                countries="${countries}"$'~~\t(padding)\n'
            done

            echo -n "${countries}" | \
                awk -F'\t' '{ printf "%d\t%s\n", (NR - 1) % '"${linesPerCol}"', $0 }' | \
                LC_ALL=C sort -n | \
                awk -F'\t' '
                    NR > 1 && !((NR - 1) % '${colsPerLine}') { printf "\n" }
                    $2 != "~~" { printf "\x1B[32m%s\x1B[0m %-'"${colWidth}"'s", $2, $3 }'
            echo
        elif [[ ! "${PI_COUNTRY}" =~ [A-Z]{2} ]]; then
            echo "Invalid country code."
            PI_COUNTRY=""
        elif ! grep -E '^'"${PI_COUNTRY}"'\t' "/usr/share/zoneinfo/iso3166.tab"; then
            echo "Unknown country code."
            PI_COUNTRY=""
        fi
    done
    echo

    expect <<EOF
        set timeout -1
        spawn ssh ${SSH_OPTS[@]} "pi@${SETUP_HOSTNAME}.local"
        expect "pi@${SETUP_HOSTNAME}"
        send "sudo su\n"
        expect "root@${SETUP_HOSTNAME}"
        # Set country settings.
        send "cat <<EOF > '/etc/wpa_supplicant/wpa_supplicant.conf'\n"
        send "country=${PI_COUNTRY}\n"
        send "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\n"
        send "update_config=1\n"
        send "EOF\n"
        expect "root@${SETUP_HOSTNAME}"

        # Backup initial configuration file for wpa_supplicant.
        send "cp '/etc/wpa_supplicant/wpa_supplicant.conf' '/etc/wpa_supplicant/wpa_supplicant.conf.orig'\n"
        expect "root@${SETUP_HOSTNAME}"

        # Unblock Wi-Fi
        send "rfkill unblock wifi\n"
        expect "root@${SETUP_HOSTNAME}"
        send "exit\n"
        expect "pi@${SETUP_HOSTNAME}"
        send "exit\n"
        expect eof
EOF

    echo "================================================================================"
    echo
    echo "CHECKPOINT 4: WLAN configured"
    echo
    echo "To restart from this point, run the following command:"
    echo "   \$ SETUP_SKIP_SD=1 SETUP_SKIP_BASE_SETUP=1 SETUP_SKIP_PACKAGES=1 SETUP_SKIP_WLAN_CONFIG=1 SETUP_HOSTNAME=$SETUP_HOSTNAME SETUP_PASSWORD=$SETUP_PASSWORD $SOURCE"
    echo "================================================================================"
fi

NEW_PI_HOSTNAME="${SETUP_HOSTNAME}"
NEW_PI_PASSWORD="${SETUP_PASSWORD}"
if [ "${SETUP_SKIP_FINALIZE:-0}" != "1" ]; then

    echo "================================================================================"
    read -r -p "Please enter hostname for Raspberry Pi ([return] to keep '${PI_HOSTNAME}'): " NEW_PI_HOSTNAME
    NEW_PI_HOSTNAME="${NEW_PI_HOSTNAME:-${PI_HOSTNAME}}"
    echo
    while [[ ! ${NEW_PI_HOSTNAME} =~ ^[a-zA-Z0-9-]*$ ]]; do
        echo "A hostname may contain only ASCII letters 'a' through 'z' (case-insensitive),"
        echo "the digits '0' through '9', and the hyphen. Please try again."
        echo
        read -r -p "Please enter hostname for Raspberry Pi : " NEW_PI_HOSTNAME
        echo
    done

    read -r -s -p "Please enter desired password for user 'pi' ([return] to keep default): " NEW_PI_PASSWORD
    NEW_PI_PASSWORD="${NEW_PI_PASSWORD:-${PI_PASSWORD}}"
    echo
    if [ -z "${NEW_PI_PASSWORD}" ] || [ "${NEW_PI_PASSWORD}" != "${PI_PASSWORD}" ]; then
        read -r -s -p "Please retype password: " NEW_PI_PASSWORD2
        echo
    else
        NEW_PI_PASSWORD2="${PI_PASSWORD}"
    fi
    while [ "${NEW_PI_PASSWORD}" != "${NEW_PI_PASSWORD2}" ]; do
        echo "Sorry, empty password or passwords do not match, please try again."
        echo
        read -r -s -p "Please enter desired password for user 'pi': " NEW_PI_PASSWORD
        echo
        if [ -n "${NEW_PI_PASSWORD}" ]; then
            read -r -s -p "Please retype password: " NEW_PI_PASSWORD2
            echo
        else
            NEW_PI_PASSWORD2="not${NEW_PI_PASSWORD}"
        fi
    done

    # Complete setup.
    echo "================================================================================"
    echo "Rebooting Raspberry Pi."
    change_hostname_and_password "${SETUP_HOSTNAME}" "${NEW_PI_HOSTNAME}" "${NEW_PI_PASSWORD}"

    echo "================================================================================"
    echo "Expand FS on next boot."
    # See /usr/bin/raspi-config.
    expect <<EOF
        set timeout -1
        spawn ssh ${SSH_OPTS[@]} "pi@${NEW_PI_HOSTNAME}.local"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "ROOT_PART=\\\$(mount | sed -n 's|^/dev/\\\(.*\\\) on / .*|\\\1|p')\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "sudo perl -i -p0e 's/\\\\n/ init=\\\/usr\\\/lib\\\/raspi-config\\\/init_resize\\\.sh\\\\n/gs' '/boot/cmdline.txt'\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "sudo perl -i -p0e 's/\\\\n/ snd_bcm2835.enable_headphones=0 snd_bcm2835.enable_hdmi=0 snd_bcm2835.enable_compat_alsa=1\\\\n/gs' '/boot/cmdline.txt'\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "cat <<EOF | sudo tee /etc/init.d/resize2fs_once\n"
        send "#!/bin/sh\n"
        send "### BEGIN INIT INFO\n"
        send "# Provides:          resize2fs_once\n"
        send "# Required-Start:\n"
        send "# Required-Stop:\n"
        send "# Default-Start: 3\n"
        send "# Default-Stop:\n"
        send "# Short-Description: Resize the root filesystem to fill partition\n"
        send "# Description:\n"
        send "### END INIT INFO\n"
        send "\n"
        send ". /lib/lsb/init-functions\n"
        send "\n"
        send "case \"\\\\\\\$1\" in\n"
        send "  start)\n"
        send "    log_daemon_msg \"Starting resize2fs_once\" &&\n"
        send "    resize2fs /dev/\\\$ROOT_PART &&\n"
        send "    update-rc.d resize2fs_once remove &&\n"
        send "    rm /etc/init.d/resize2fs_once &&\n"
        send "    log_end_msg \\\\\\\$?\n"
        send "    ;;\n"
        send "  *)\n"
        send "    echo \"Usage: \\\\\\\$0 start\" >&2\n"
        send "    exit 3\n"
        send "    ;;\n"
        send "esac\n"
        send "EOF\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "sudo chmod +x /etc/init.d/resize2fs_once\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "sudo update-rc.d resize2fs_once defaults\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "exit\n"
        expect eof
EOF

    # Write raspbian package time stamp.
    echo "================================================================================"
    echo "Write raspbian package time stamp."
    currentTime="$(date -u)"
    expect <<EOF
        set timeout -1
        spawn ssh ${SSH_OPTS[@]} "pi@${NEW_PI_HOSTNAME}.local"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "sudo su\n"
        expect "root@${NEW_PI_HOSTNAME}"
        send "date --date='${currentTime}' -u --iso-8601 > /etc/raspbian-package-timestamp\n"
        expect "root@${NEW_PI_HOSTNAME}"
        send "exit\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "exit\n"
        expect eof
EOF

    # Delete history.
    echo "================================================================================"
    echo "Delete history."
    expect <<EOF
        set timeout -1
        spawn ssh ${SSH_OPTS[@]} "pi@${NEW_PI_HOSTNAME}.local"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "sudo su\n"
        expect "root@${NEW_PI_HOSTNAME}"
        send "history -cw\n"
        expect "root@${NEW_PI_HOSTNAME}"
        send "exit\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "rm -rf ~/.ssh\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "sudo rm /root/.bash_history\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "rm ~/.bash_history\n"
        expect "pi@${NEW_PI_HOSTNAME}"
        send "history -cw && sudo halt\n"
        expect eof
EOF

    rm -rf .tmp
    rm -rf "${SSH_TMP}"

    echo "================================================================================"
    echo
    echo "CHECKPOINT 5: Setup complete."
    echo
    echo "To restart from this point, run the following command:"
    echo "   \$ SETUP_SKIP_SD=1 SETUP_SKIP_BASE_SETUP=1 SETUP_SKIP_PACKAGES=1 SETUP_SKIP_WLAN_CONFIG=1 SETUP_SKIP_FINALIZE=1 SETUP_HOSTNAME=$NEW_PI_HOSTNAME SETUP_PASSWORD=$NEW_PI_PASSWORD $SOURCE"
    echo "================================================================================"
fi

if [ "${SETUP_SKIP_SAVE:-0}" != "1" ]; then
    echo
    echo "================================================================================"
    echo "Save Raspberry Pi image"
    echo
    echo "Saves a copy of the fully configured image for backup and/or cloning."
    echo "================================================================================"
    echo "Please enter name for Raspberry Pi image file ([return] to skip creating image)."
    read -r -p "File name: " imageFileName
    if [[ -n "${imageFileName}" ]]; then
        create_image "${imageFileName}" "${SD_SIZE_MB}"
        echo "SD card image with size ${SD_SIZE_MB} MB written to file:"
        echo "${imageFileName}"
        echo "================================================================================"
    else
        echo "Skipped image creation."
        echo "Power cycle Raspi, linux partition will be resized to full SD card."
        echo "================================================================================"
        echo "Log in with \`ssh pi@${NEW_PI_HOSTNAME}.local\` and the previously supplied password."
        echo "================================================================================"
    fi
fi

if [ "${SETUP_SKIP_IMPORT_SDK:-0}" != "1" ]; then
    echo
    echo "================================================================================"
    echo "Import Raspberry Pi header files and libraries"
    echo
    echo "In order to build Raspberry Pi application on your computer, you need to import"
    echo "certain header files and libraries from the Raspberry Pi into your computer."
    echo "Those files will be imported into External/raspi-sdk directory."
    echo
    echo "To restart from this point, run the following command:"
    echo "   \$ SETUP_SKIP_SD=1 SETUP_SKIP_BASE_SETUP=1 SETUP_SKIP_PACKAGES=1 SETUP_SKIP_WLAN_CONFIG=1 SETUP_SKIP_FINALIZE=1 SETUP_SKIP_SAVE=1 NEW_PI_HOSTNAME=$NEW_PI_HOSTNAME NEW_PI_PASSWORD=$NEW_PI_PASSWORD $SOURCE"

    echo "================================================================================"
    read -r -p "Import header files from Raspberry Pi? [y/N]" CONFIRM
    echo
    [ "$CONFIRM" == "y" ] || [ "$CONFIRM" == "Y" ]
    echo
    echo "================================================================================"
    echo "1. Put SD card into Raspberry Pi."
    echo "2. Connect Raspberry Pi to host via Ethernet (raspi3 & 4) / USB (raspi zero)."
    echo "3. Power Raspberry Pi."
    read -r -s -p "Press [return] to continue."
    echo
    echo "================================================================================"
    wait_for_reboot "$NEW_PI_HOSTNAME.local" 60
    "$ADK_ROOT"/Tools/raspi_sdk_import.py -n "$NEW_PI_HOSTNAME" -p "$NEW_PI_PASSWORD"
fi

echo
echo "================================================================================"
echo
echo "FINAL CHECKPOINT: All done!"
echo
echo "================================================================================"
echo
