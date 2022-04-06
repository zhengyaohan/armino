#!/bin/bash -e
# shellcheck disable=SC2155 # Declare and assign separately to avoid masking return values.

ADK_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../"
HOST="$(uname)"

fail() {
    echo "$1"
    echo 'Use Provision --help to display usage information.'
    false
}

if [[ -z "${1:-}" ]] || [[ "${1:-}" == "-h" ]] || [[ "${1:-}" == "--help" ]]; then
    echo 'HomeKit Accessory Setup Provisioning Tool for Nordic boards'
    echo ''
    echo 'USAGE'
    echo '     Provision [OPTION]... <Destination key-value store hex file> [<Base Address>]'
    echo ''
    echo 'DESCRIPTION'
    echo '     This tool generates and provisions accessory setup information for HomeKit.'
    echo ''
    echo '     EACH ACCESSORY NEEDS TO BE PROVISIONED WITH UNIQUE ACCESSORY SETUP'
    echo '     INFORMATION BEFORE IT MAY BE USED.'
    echo ''
    echo '     Destination key-value store hex file is the location where a new key-value'
    echo '     store is produced that is provisioned with the generated accessory setup'
    echo '     information. This hex file can then be flashed to the accessory.'
    echo ''
    echo '     The --category, --display and --nfc options must match the accessory'
    echo '     capabilities and configuration.'
    echo ''
    echo '     Base Address denotes the base flash address at which FDS starts.'
    echo '     Format is XXXXXXXX with X being a hex digit.'
    echo '     - nRF52832 (PCA10040): 0007E000'
    echo '     - nRF52840 (PCA10056): 000FE000'
    echo '     See function flash_end_addr in fds.c of nRF5 SDK.'
    echo ''
    echo 'OPTIONS'
    echo '     The following options are available:'
    echo ''
    echo '     --display'
    echo '        Accessory is connected to a display that supports showing a setup code.'
    echo '        A random setup code is generated for each pairing attempt, so a fixed'
    echo '        setup code cannot be specified if this option is selected.'
    echo ''
    echo '     --nfc'
    echo '        Accessory has a programmable NFC tag.'
    echo ''
    echo '     --category <Category>'
    echo '        The accessory category.'
    echo ''
    echo '        An accessory with support for multiple categories should advertise the'
    echo '        primary category. An accessory for which a primary category cannot be'
    echo '        determined or the primary category isn'"'"'t among the well defined'
    echo '        categories falls in the Other category.'
    echo ''
    echo '        Well defined categories:'
    echo '          1  Other.'
    echo '          2  Bridges.'
    echo '          3  Fans.'
    echo '          4  Garage Door Openers.'
    echo '          5  Lighting.'
    echo '          6  Locks.'
    echo '          7  Outlets.'
    echo '          8  Switches.'
    echo '          9  Thermostats.'
    echo '         10  Sensors.'
    echo '         11  Security Systems.'
    echo '         12  Doors.'
    echo '         13  Windows.'
    echo '         14  Window Coverings.'
    echo '         15  Programmable Switches.'
    echo '         16  Range Extenders.'
    echo '         17  IP Cameras.'
    echo '         18  Video Doorbells.'
    echo '         19  Air Purifiers.'
    echo '         20  Heaters.'
    echo '         21  Air Conditioners.'
    echo '         22  Humidifiers.'
    echo '         23  Dehumidifiers.'
    echo '         24  Apple TV.'
    echo '         28  Sprinklers.'
    echo '         29  Faucets.'
    echo '         30  Shower Systems.'
    echo '         32  Remotes.'
    echo ''
    echo '     --setup-code <Setup code>'
    echo '        Provisions accessory setup information that allows pairing using the'
    echo '        specified setup code (e.g. for development).'
    echo '        Format is XXX-XX-XXX with X being a digit from 0-9.'
    echo '        - Setup codes that only consist of a repeating digit are not allowed.'
    echo '        - Example: 123-45-678 and 876-54-321 are not allowed.'
    echo '        If this option is not present, a random setup code is provisioned.'
    echo ''
    echo '     --setup-id <Setup ID>'
    echo '        Provisions accessory setup information using a specific setup ID.'
    echo '        Format is XXXX with X being a digit from 0-9 or a character from A-Z.'
    echo '        Setup ID and EUI are mutually exclusive:  Accessories may be provisioned with'
    echo '        one or the other, but not both'
    echo '        - Lowercase characters are not allowed.'
    echo '        If this option is not present, and no EUI is provided,'
    echo '        a random setup ID is provisioned.'
    echo ''
    echo '     -- eui <EUI64>'
    echo '        Uses EUI to generate QR code and Setup ID'
    echo '        format is the 64 bit value in hex.'
    echo '        - Lowercase characters are not allowed.'
    echo '        - EUI and setup id are mutually exclusive.'
    echo ''
    echo '     --product-data <Product Data>'
    echo '        Provisions accessory setup information with a specific Product Data.'
    echo '        - Format is the 64 bit value represented in hexidecimal.'
    echo '        - Top 32 bits are the Product Group, Lower 32 bits are the Product Number'
    echo '        - Lowercase characters are not allowed'
    echo ''
    echo '     --mfi-token <Software Token UUID> <Software Token>'
    echo '        Provisions information required for Software Authentication.'
    echo '        Format for <Software Token UUID> is XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX'
    echo '        with X being a digit from 0-9 or a character from A-Z.'
    echo '        Format for <Software Token> is base64.'
    echo '        The <Software Token UUID> and initial <Software Token> are deployed'
    echo '        to the key-value store.'
    echo ''
    echo '     --fds-base <FDS File ID Base> <FDS Record ID Base>'
    echo '        The base of the FDS file and record ranges used by the key-value store.'
    echo '        This must match the values passed to HAPPlatformKeyValueStoreCreate.'
    echo '        Format of each ID base is XXXX with X being a hex digit.'
    echo '        If this option is not present, BF00 is used for both ID bases.'
    echo ''
    echo '     --install'
    echo '        Install the provisioning file on the connected device.'
    echo ''
    echo 'OUTPUT'
    echo '     The destination key-value store is provisioned with information that is'
    echo '     compatible with the provided HAP platform implementation. If that'
    echo '     implementation is modified, this tool may need to be modified as well.'
    echo '     It is left up to the accessory manufacturer to implement a production ready'
    echo '     provisioning concept.'
    echo ''
    echo '     Upon completion, information to be printed on labels is printed to stdout.'
    echo '     These labels must be affixed to the accessory and its packaging.'
    echo '     If an accessory is connected to a display, random setup information is used'
    echo '     for every pairing attempt, and no label information is generated.'
    echo ''
    echo '     If a QR code label is supported (no display or programmable NFC tag),'
    echo '     it is printed to stdout using ANSI characters (requires qrencode).'
    echo '     This QR code is to be used for development purposes only!'
    echo '     Please refer to Works with Apple HomeKit Identity Guidelines for more'
    echo '     details on identity guidelines for including QR codes on the accessory.'
    exit
fi

################################################################################
# Configuration.
################################################################################
sdkDomainsFile="$ADK_ROOT/PAL/nRF52/HAPPlatformKeyValueStore+SDKDomains.h"

################################################################################
# Parse command line.
################################################################################
supportsDisplay=0
supportsProgrammableNFC=0
category=0
fixedSetupCode=""
fixedSetupID=""
fixedProductData=""
fixedEui=""
mfiTokenUUID=""
mfiToken=""
fdsFileIDBase=""
fdsRecordIDBase=""
destination=""
baseAddress=""
transport=0
install=""
ble_transport=0
ip_transport=0
thread_transport=0
while (( $# )); do
    case "${1}" in
        "--display")
            shift
            if (( supportsDisplay )); then
                fail "--display specified multiple times."
            fi
            supportsDisplay=1
            ;;

        "--nfc")
            shift
            if (( supportsProgrammableNFC )); then
              fail "--nfc specified multiple times."
            fi
            supportsProgrammableNFC=1
            ;;

        "--category")
            shift
            if (( category )); then
              fail "--category specified multiple times."
            fi
            if (( $# < 1 )); then
              fail "--category specified without accessory category identifier."
            fi
            categoryID="${1}"
            shift
            if [ ! "${categoryID}" -eq "${categoryID}" ]; then
              fail "--category specified with malformed accessory category identifier."
            fi
            if (( ! categoryID )) || (( categoryID > 255 )); then
              fail "--category specified with out-of-range accessory category identifier."
            fi
            category=${categoryID}
            ;;

        "--setup-code")
            shift
            if [ "${fixedSetupCode}" != "" ]; then
              fail "--setup-code specified multiple times."
            fi
            if (( $# < 1 )); then
              fail "--setup-code specified without setup code."
            fi
            fixedSetupCode="${1}"
            shift
            ;;

        "--eui")
            shift
            if [ "${fixedEui}" != "" ]; then
              fail "--eui specified multiple times."
            fi
            if (( $# < 1 )); then
              fail "--eui specified without eui."
            fi
            fixedEui="${1}"
            shift
            ;;

        "--setup-id")
            shift
            if [ "${fixedSetupID}" != "" ]; then
              fail "--setup-id specified multiple times."
            fi
            if (( $# < 1 )); then
              fail "--setup-id specified without setup ID."
            fi
            fixedSetupID="${1}"
            shift
            ;;

        "--product-data")
            shift
            if [ "${fixedProductData}" != "" ]; then
              fail "--product-data specified multiple times."
            fi
            if (( $# < 1 )); then
              fail "--product-data specified without setup ID."
            fi
            fixedProductData="${1}"
            shift
            ;;

        "--mfi-token")
            shift
            if [ "${mfiTokenUUID}" != "" ] || [ "${mfiToken}" != "" ]; then
              fail "--mfi-token specified multiple times."
            fi
            if (( $# < 2 )); then
              fail "--mfi-token specified without Software Token UUID or Software Token."
            fi
            mfiTokenUUIDString="${1}"
            mfiTokenBase64String="${2}"
            shift 2
            mfiTokenUUIDRegex=""
            mfiTokenUUIDRegex="${mfiTokenUUIDRegex}([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})-"
            mfiTokenUUIDRegex="${mfiTokenUUIDRegex}([0-9A-F]{2})([0-9A-F]{2})-([0-9A-F]{2})([0-9A-F]{2})-"
            mfiTokenUUIDRegex="${mfiTokenUUIDRegex}([0-9A-F]{2})([0-9A-F]{2})-([0-9A-F]{2})([0-9A-F]{2})"
            mfiTokenUUIDRegex="${mfiTokenUUIDRegex}([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2})"
            if [[ ! "${mfiTokenUUIDString}" =~ ${mfiTokenUUIDRegex} ]]; then
              fail "--mfi-token specified with malformed Software Token UUID."
            fi
            # Reversed mfiTokenUUID network byte order.
            mfiTokenUUID=""
            mfiTokenUUID="${mfiTokenUUID}${BASH_REMATCH[16]}${BASH_REMATCH[15]}${BASH_REMATCH[14]}${BASH_REMATCH[13]}"
            mfiTokenUUID="${mfiTokenUUID}${BASH_REMATCH[12]}${BASH_REMATCH[11]}${BASH_REMATCH[10]}${BASH_REMATCH[ 9]}"
            mfiTokenUUID="${mfiTokenUUID}${BASH_REMATCH[ 8]}${BASH_REMATCH[ 7]}${BASH_REMATCH[ 6]}${BASH_REMATCH[ 5]}"
            mfiTokenUUID="${mfiTokenUUID}${BASH_REMATCH[ 4]}${BASH_REMATCH[ 3]}${BASH_REMATCH[ 2]}${BASH_REMATCH[ 1]}"
            if ! mfiToken="$(base64 --decode <<< "${mfiTokenBase64String}" | xxd -ps -u | tr -d '\n')"; then
              fail "--mfi-token specified with malformed Software Token."
            fi
            ;;

        "--fds-base")
            shift
            if [ "${fdsFileIDBase}" != "" ] || [ "${fdsRecordIDBase}" != "" ]; then
              fail "--fds-base specified multiple times."
            fi
            if (( $# < 2 )); then
              fail "--fds-base specified without FDS File ID Base or FDS Record ID Base."
            fi
            fdsFileIDBase="${1}"
            fdsRecordIDBase="${2}"
            shift 2
            if [[ ! "${fdsFileIDBase}" =~ [0-9a-fA-F]{4} ]]; then
              fail "--fds-base specified with malformed FDS File ID Base."
            fi
            if [[ ! "${fdsRecordIDBase}" =~ [0-9a-fA-F]{4} ]]; then
              fail "--fds-base specified with malformed FDS Record ID Base."
            fi
            fdsFileIDBase=$((16#$fdsFileIDBase))
            fdsRecordIDBase=$((16#$fdsRecordIDBase))
            ;;

        "--transport")
            shift
            if(( $# < 1)); then
              fail "--transport specified without value."
            fi
            transport=1
            case ${1} in
              ble) ble_transport=1;;
              ip) ip_transport=1;;
              thread) thread_transport=1;;
              *) fail "--transport specified with invalid param[${1}]"
              esac
            shift
            ;;

        "--install")
            shift
            if (( install )); then
              fail "--install specified multiple times."
            fi
            install=1
            ;;

        *)
            if [ "${baseAddress}" != "" ]; then
              fail "Too many arguments specified."
            fi
            if [ "${destination}" == "" ]; then
              destination="${1}"
            else
              baseAddress="${1}"
              if [[ ! "${baseAddress}" =~ [0-9a-fA-F]{8} ]]; then
                fail "Invalid Base Address specified."
              fi
              baseAddress=$((16#$baseAddress))
            fi
            shift
            ;;

  esac
done
if (( ! category )); then
    fail "--category not specified."
fi
if (( supportsDisplay )) && [ "${fixedSetupCode}" != "" ]; then
    fail "--setup-code cannot be specified if --display is selected."
fi
if [ "${destination}" == "" ]; then
    fail "Destination key-value store not specified."
fi
if [ "${baseAddress}" == "" ]; then
    fail "Base Address not specified."
fi
if [ "${fdsFileIDBase}" == "" ] || [ "${fdsRecordIDBase}" == "" ]; then
    fdsFileIDBase=$((16#BF00))
    fdsRecordIDBase=$((16#BF00))
fi

# If no transports were provided then BLE is the default
if (( transport == 0 )); then
    ble_transport=1
fi

if [ "${fixedProductData}" == "" ]; then
    fail "product number not specified."
fi

(
    # Make all the tools needed by provisioning script
    pushd "$ADK_ROOT" > /dev/null
    make tools
)

accessorySetupGenerator=$(find "$ADK_ROOT"/Output/"$HOST"-* -name AccessorySetupGenerator.OpenSSL)
if [[ -z "$accessorySetupGenerator" ]]; then
    echo "Error: AccessorySetupGenerator tool isn't available"
    exit
fi

################################################################################
# Generate accessory setup information.
################################################################################
flags=' --category '"${category}"
if ((ble_transport == 1)); then
   flags="${flags} --ble"
fi
if ((ip_transport == 1)); then
   flags="${flags} --ip"
fi

if ((thread_transport == 1)); then
    eui=
    if [ "${fixedEui}" != "" ]; then
       eui="$fixedEui"
    else
       eui="0000000000000000"
    fi
    flags="${flags}"' --thread --eui '"${eui}"
fi

flags="${flags}"
if [ "${fixedSetupCode}" != "" ]; then
    flags="${flags}"' --setup-code '"${fixedSetupCode}"
fi
if [ "${fixedSetupID}" != "" ]; then
    flags="${flags}"' --setup-id '"${fixedSetupID}"
fi

flags="${flags}"' --product-data '"${fixedProductData}"

echo "AccessorySetupGenerator run with flags""[${flags}]"

# shellcheck disable=SC2086
accessorySetup=$("${accessorySetupGenerator}" ${flags})
# shellcheck disable=SC2206
accessorySetup=(${accessorySetup})

accessorySetupVersion="${accessorySetup[0]}"
if [ "${accessorySetupVersion}" != "1" ]; then
    fail "Incompatible with Accessory Setup Generator."
fi

setupCode="${accessorySetup[1]}"
srpSalt="${accessorySetup[2]}"
srpVerifier="${accessorySetup[3]}"
setupID="${accessorySetup[4]}"
setupPayload="${accessorySetup[5]}"
joinerPassphrase="${accessorySetup[6]}"

################################################################################
# Provision accessory setup information.
################################################################################
provisioningDomainID="$(grep "#define kSDKKeyValueStoreDomain_Provisioning " "${sdkDomainsFile}")"
[[ "${provisioningDomainID}" =~ "(HAPPlatformKeyValueStoreDomain) "0x([0-9]+) ]]
provisioningDomain="${BASH_REMATCH[1]}"

setupInfoKeyID="$(grep "#define kSDKKeyValueStoreKey_Provisioning_SetupInfo " "${sdkDomainsFile}")"
[[ "${setupInfoKeyID}" =~ "(HAPPlatformKeyValueStoreKey) "0x([0-9]+) ]]
setupInfoKey="${BASH_REMATCH[1]}"

setupIDKeyID="$(grep "#define kSDKKeyValueStoreKey_Provisioning_SetupID " "${sdkDomainsFile}")"
[[ "${setupIDKeyID}" =~ "(HAPPlatformKeyValueStoreKey) "0x([0-9]+) ]]
setupIDKey="${BASH_REMATCH[1]}"

setupCodeKeyID="$(grep "#define kSDKKeyValueStoreKey_Provisioning_SetupCode " "${sdkDomainsFile}")"
[[ "${setupCodeKeyID}" =~ "(HAPPlatformKeyValueStoreKey) "0x([0-9]+) ]]
setupCodeKey="${BASH_REMATCH[1]}"

joinerPassKeyID="$(grep "#define kSDKKeyValueStoreKey_Provisioning_JoinerPassphrase " "${sdkDomainsFile}")"
[[ "${joinerPassKeyID}" =~ "(HAPPlatformKeyValueStoreKey) "0x([0-9]+) ]]
joinerKey="${BASH_REMATCH[1]}"

mfiTokenUUIDKeyID="$(grep "#define kSDKKeyValueStoreKey_Provisioning_MFiTokenUUID " "${sdkDomainsFile}")"
[[ "${mfiTokenUUIDKeyID}" =~ "(HAPPlatformKeyValueStoreKey) "0x([0-9]+) ]]
mfiTokenUUIDKey="${BASH_REMATCH[1]}"

mfiTokenKeyID="$(grep "#define kSDKKeyValueStoreKey_Provisioning_MFiToken " "${sdkDomainsFile}")"
[[ "${mfiTokenKeyID}" =~ "(HAPPlatformKeyValueStoreKey) "0x([0-9]+) ]]
mfiTokenKey="${BASH_REMATCH[1]}"

# $1: Type.
# $2: Address.
# $3: Data as hex string.
hexLine() {
    (( ${1} >= 0 )) && (( ${1} <= 0xFF ))
    (( ${2} >= 0 )) && (( ${2} <= 0xFFFF ))
    [[ "${3}" =~ ^([0-9a-fA-F]{2})*$ ]]

    local sum=0
    printf ":%02X%04X%02X" $(( ${#3} / 2 )) "${2}" "${1}"
    sum=$(( sum + (${#3} / 2) ))
    sum=$(( sum + (${2} >> 0 & 0xFF) ))
    sum=$(( sum + (${2} >> 8 & 0xFF) ))
    sum=$(( sum + (${1} & 0xFF) ))
    local i
    for (( i=0; i < ${#3} / 2; i++ )); do
        c=$(( 16#${3:$(( 2 * i )):2} ))
        printf "%02X" ${c}
        sum=$(( sum + c ))
    done
    printf "%02X\r\n" $(( -sum & 0xFF ))
}

# Create hex file.
address=$(( baseAddress ))
addressHi=$(( address >> 0x10 & 0xFFFF ))
addressLo=$(( address >> 0x00 & 0xFFFF ))
echo -n "" > "${destination}"

# High address.
hexLine 4 0 "$(printf '%02X%02X' $(( addressHi >> 8 & 0xFF )) $(( addressHi >> 0 & 0xFF )))" >> "${destination}"
a=$(( addressLo ))

# Page tag.
pageTagHi=$(( 0xF11E01FE ))
pageTagLo=$(( 0xDEADC0DE ))
pageTagBytes="$(printf '%02X%02X%02X%02X%02X%02X%02X%02X' \
$(( pageTagLo >> 0x00 & 0xFF )) \
$(( pageTagLo >> 0x08 & 0xFF )) \
$(( pageTagLo >> 0x10 & 0xFF )) \
$(( pageTagLo >> 0x18 & 0xFF )) \
$(( pageTagHi >> 0x00 & 0xFF )) \
$(( pageTagHi >> 0x08 & 0xFF )) \
$(( pageTagHi >> 0x10 & 0xFF )) \
$(( pageTagHi >> 0x18 & 0xFF )))"
hexLine 0 ${a} "${pageTagBytes}" >> "${destination}"
a=$(( a + ${#pageTagBytes} / 2 ))
recordID=$(( 0 ))

# $1: Old CRC value, or 0xFFFF.
# $2: Value as hex string.
# Output: New CRC value.
crc16() {
    (( ${1} >= 0 )) && (( ${1} <= 0xFFFF ))
    [[ "${2}" =~ ^([0-9a-fA-F]{2})*$ ]]

    local crc=$(( ${1} ))

    local i
    for (( i = 0; i < ${#2} / 2; i++ )); do
        local c=$(( 16#${2:$(( 2 * i )):2} ))
        crc=$(( ((crc >> 8 & 0xFF) | (crc << 8)) & 0xFFFF ))
        crc=$(( (crc ^ c) & 0xFFFF ))
        crc=$(( (crc ^ ((crc & 0xFF) >> 4)) & 0xFFFF ))
        crc=$(( (crc ^ (crc << 8) << 4) & 0xFFFF ))
        crc=$(( (crc ^ ((crc & 0xFF) << 4) << 1) & 0xFFFF ))
    done

    printf '%u' ${crc}
}

# $1: HAPPlatformKeyValueStoreDomain.
# $2: HAPPlatformKeyValueStoreKey.
# $3: Value as hex string.
# Global variables: a, recordID.
writeRecord() {
    (( ${1} >= 0 )) && (( ${1} <= 0xFF ))
    (( ${2} >= 0 )) && (( ${2} <= 0xFF ))
    [[ "${3}" =~ ^([0-9a-fA-F]{2})*$ ]]

    # Record key.
    local recordKey=$(( fdsRecordIDBase + ${2} ))
    local recordKeyBytes="$(printf '%02X%02X' \
    $(( recordKey >> 0x00 & 0xFF )) \
    $(( recordKey >> 0x08 & 0xFF )))"

    # Data length.
    local numBytes=$(( ${#3} / 2 ))
    local numWords=$(( (( 1 + numBytes) + 3) / 4 ))
    local numPaddedBytes=$(( numWords * 4 - (1 + numBytes) ))
    (( numPaddedBytes < 4 ))
    local dataLengthBytes="$(printf '%02X%02X' \
    $(( numWords >> 0x00 & 0xFF )) \
    $(( numWords >> 0x08 & 0xFF )))"

    # File ID.
    local fileID=$(( fdsFileIDBase + ${1} ))
    local fileIDBytes="$(printf '%02X%02X' \
    $(( fileID >> 0x00 & 0xFF )) \
    $(( fileID >> 0x08 & 0xFF )))"

    # Record ID.
    recordID=$(( recordID + 1 ))
    local recordIDBytes="$(printf '%02X%02X' \
    $(( recordID >> 0x00 & 0xFF )) \
    $(( recordID >> 0x08 & 0xFF )) \
    $(( recordID >> 0x10 & 0xFF )) \
    $(( recordID >> 0x18 & 0xFF )))"

    # Padding.
    local paddingBytes=""
    local i
    for (( i = 1; i <= 3; i++ )); do
        if (( numPaddedBytes >= i )); then
            paddingBytes="${paddingBytes}00"
        fi
    done

    # CRC.
    local crc=$(( 0xFFFF ))
    crc=$(( $(crc16 ${crc} "${recordKeyBytes}") ))
    crc=$(( $(crc16 ${crc} "${dataLengthBytes}") ))
    crc=$(( $(crc16 ${crc} "${fileIDBytes}") ))
    crc=$(( $(crc16 ${crc} "${recordIDBytes}") ))
    crc=$(( $(crc16 ${crc} "$(printf '%02X' ${numPaddedBytes})") ))
    crc=$(( $(crc16 ${crc} "${3}") ))
    crc=$(( $(crc16 ${crc} "${paddingBytes}") ))
    local crcBytes="$(printf '%02X%02X' \
    $(( crc >> 0x00 & 0xFF )) \
    $(( crc >> 0x08 & 0xFF )))"

    # Content.
    hexLine 0 ${a} "${recordKeyBytes}"
    a=$(( a + ${#recordKeyBytes} / 2 ))
    hexLine 0 ${a} "${dataLengthBytes}"
    a=$(( a + ${#dataLengthBytes} / 2 ))
    hexLine 0 ${a} "${fileIDBytes}"
    a=$(( a + ${#fileIDBytes} / 2 ))
    hexLine 0 ${a} "${crcBytes}"
    a=$(( a + ${#crcBytes} / 2 ))
    hexLine 0 ${a} "${recordIDBytes}"
    a=$(( a + ${#recordIDBytes} / 2 ))
    hexLine 0 ${a} "$(printf '%02X' ${numPaddedBytes})"
    a=$(( a + 1 ))
    for (( i = 0; i < numBytes; i += 16 )); do
        if (( numBytes - i > 16 )); then
            hexLine 0 ${a} "${3:$(( 2 * i )):32}"
            a=$(( a + 16 ))
        else
            hexLine 0 ${a} "${3:$(( 2 * i ))}"
            a=$(( a + (numBytes - i) ))
        fi
    done
    hexLine 0 ${a} "${paddingBytes}"
    a=$(( a + ${#paddingBytes} / 2 ))
}

if (( ! supportsDisplay )); then
    writeRecord $(( 16#${provisioningDomain} )) $(( 16#${setupInfoKey} )) \
    "${srpSalt}${srpVerifier}" >> "${destination}"
    if (( supportsProgrammableNFC )); then
        writeRecord $(( 16#${provisioningDomain} )) $(( 16#${setupCodeKey} )) \
        "$(echo -en "${setupCode}"'\0' | xxd -ps -u | tr -d '\n')" >> "${destination}"
    fi
    writeRecord $(( 16#${provisioningDomain} )) $(( 16#${joinerKey} )) \
    "$(echo -en "${joinerPassphrase}"'\0' | xxd -ps -u | tr -d '\n')" >> "${destination}"
fi
writeRecord $(( 16#${provisioningDomain} )) $(( 16#${setupIDKey} )) \
"$(echo -en "${setupID}"'\0' | xxd -ps -u | tr -d '\n')" >> "${destination}"
if [ "${mfiTokenUUID}" != "" ] && [ "${mfiToken}" != "" ]; then
    writeRecord $(( 16#${provisioningDomain} )) $(( 16#${mfiTokenUUIDKey} )) "${mfiTokenUUID}" >> "${destination}"
    writeRecord $(( 16#${provisioningDomain} )) $(( 16#${mfiTokenKey} )) "${mfiToken}" >> "${destination}"
fi

hexLine 1 0 "" >> "${destination}"

################################################################################
# Display information to print on label.
################################################################################
displayPayload=$((1-supportsDisplay))
if [[ thread_transport -eq 1 && "${fixedEui}" == "" ]]; then
    echo "================================================================================"
    echo "NOTE:  EUI not provided for Thread transport.  Cannot generate setup payload"
    echo "================================================================================"
    displayPayload=0
fi

if (( displayPayload )); then
    echo "================================================================================"
    echo " NOTE: The following setup payload is only valid if accurate Setup Code, EUI,"
    echo "       and Product Data were provided."
    echo ""
    echo "                             Product Data: ${fixedProductData}"
    echo "                             EUI: ${fixedEui}"
    echo "                             Setup code: ${setupCode}"
    echo "             Setup payload: ${setupPayload}"
    echo "================================================================================"
    if command -v qrencode > /dev/null; then
        qrencode -t ANSIUTF8 -m 2 "${setupPayload}"
    else
        echo "WARNING: qrencode utility not installed. Not generating QR code."
    fi
    echo "================================================================================"
    if ((supportsProgrammableNFC)); then
        if command -v qrencode > /dev/null; then
            nfcLabel=""
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel}"'         XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX         \n'
            nfcLabel="${nfcLabel}"'        XXX                                   XXX        \n'
            nfcLabel="${nfcLabel}"'        XX                                     XX        \n'
            nfcLabel="${nfcLabel}"'        XX                                     XX        \n'
            nfcLabel="${nfcLabel}"'        XX                      XX             XX        \n'
            nfcLabel="${nfcLabel}"'        XX                 XX    XX            XX        \n'
            nfcLabel="${nfcLabel}"'        XX             XX   XX    XX           XX        \n'
            nfcLabel="${nfcLabel}"'        XX         XX   XX   XX    XX          XX        \n'
            nfcLabel="${nfcLabel}"'        XX          XX   XX   XX   XX          XX        \n'
            nfcLabel="${nfcLabel}"'        XX           XX   XX   XX   XX         XX        \n'
            nfcLabel="${nfcLabel}"'        XX           XX   XX   XX   XX         XX        \n'
            nfcLabel="${nfcLabel}"'        XX           XX   XX   XX   XX         XX        \n'
            nfcLabel="${nfcLabel}"'        XX          XX   XX   XX   XX          XX        \n'
            nfcLabel="${nfcLabel}"'        XX         XX   XX   XX    XX          XX        \n'
            nfcLabel="${nfcLabel}"'        XX             XX   XX    XX           XX        \n'
            nfcLabel="${nfcLabel}"'        XX                 XX    XX            XX        \n'
            nfcLabel="${nfcLabel}"'        XX                      XX             XX        \n'
            nfcLabel="${nfcLabel}"'        XX                                     XX        \n'
            nfcLabel="${nfcLabel}"'        XX                                     XX        \n'
            nfcLabel="${nfcLabel}"'        XXX                                   XXX        \n'
            nfcLabel="${nfcLabel}"'         XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX         \n'
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel}"'                                                         \n'
            nfcLabel="${nfcLabel// /\\x1B[48;5;231m \\x1B[0m}"
            nfcLabel="${nfcLabel//X/\\x1B[48;5;16m \\x1B[0m}"
            echo -e -n "${nfcLabel}"
            echo "================================================================================"
        fi
    fi
fi

# Install the provisioning file on the device
if (( install )); then
    echo "Installing provisioning file..."
    jlink=$(find "$ADK_ROOT"/Output/"$HOST"-* -name jlink)
    "$jlink" \
        --select 0 \
        --run \
        "$destination"
fi
