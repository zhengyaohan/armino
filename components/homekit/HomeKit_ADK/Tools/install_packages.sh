#!/bin/bash -e
set -eu -o pipefail
GREEN="\033[0;32m"
RED="\033[0;31m"
NO_COLOR="\033[0m"

checkResult(){
    if [ "${1}" -eq 0 ]; then
        echo -e "${GREEN}${2} was successful${NO_COLOR}"
    else
        echo -e "${RED}${2} was unsuccessful${NO_COLOR}"
        exit 1
    fi
}

# List of current dependency packages and their usage:
#   Network         hostapd, dnsmasq
#   USB             libusb-1.0, libusb-dev
#   QR code         qrencode
#   Audio           libopus-dev, libasound2-dev
#   Camera          libraspberrypi-dev, raspberrypi-kernel-headers
#   Database        libsqlite3-dev
DEP_PKGS=("hostapd"
    "dnsmasq"
    "libusb-1.0"
    "libusb-dev"
    "qrencode"
    "libopus-dev"
    "libasound2-dev"
    "libraspberrypi-dev"
    "raspberrypi-kernel-headers"
    "libsqlite3-dev"
    "libglib2.0-dev")

# Build dependencies and useful tools packages
BLD_PKGS=("clang" "cmake" "ccache" "git" "valgrind" "vim" "tmux" "automake" "debhelper" "unifdef" "zip" "golang")

echo
echo "================================================================================"
echo "Installing dependencies"
echo "================================================================================"
sudo apt-get update --allow-releaseinfo-change
checkResult $? "Line Number: ${LINENO} - Apt Update"
# Install DEP Packages.
for pkg in "${DEP_PKGS[@]}"; do
    sudo apt-get install -y "${pkg}"
    checkResult $? "Line Number: ${LINENO} - Installing ${pkg}"
done
# Install Dev tool packages.
for pkg in "${BLD_PKGS[@]}"; do
    sudo apt-get install -y "${pkg}"
    checkResult $? "Line Number: ${LINENO} - Installing ${pkg}"
done

# install OpenSSL from source
echo
echo "================================================================================"
echo "Installing OpenSSL."
echo "================================================================================"
rm -fr openssl
VERSION_OPENSSL="OpenSSL_1_1_1" # https://github.com/ARMmbed/mbedtls
git clone --recursive --depth 1 --branch "${VERSION_OPENSSL}" https://github.com/openssl/openssl.git
checkResult $? "Line Number: ${LINENO} - Downloading OpenSSL"
pushd openssl > /dev/null
rm -fr crypto/idea crypto/mdc2 crypto/rc5
./config no-idea no-mdc2 no-rc5 --prefix=/usr --openssldir=/etc/ssl --libdir=/usr/lib/arm-linux-gnueabihf && make
checkResult $? "Line Number: ${LINENO} - Building OpenSSL"
sudo make install
checkResult $? "Line Number: ${LINENO} - Installing OpenSSL"
popd > /dev/null
rm -fr openssl

# install MbedTLS from source
echo
echo "================================================================================"
echo "Installing MbedTLS."
echo "================================================================================"
rm -fr mbedtls
VERSION_MBEDTLS="2.18.1" # https://github.com/ARMmbed/mbedtls
git clone --recursive --depth 1 --branch mbedtls-"${VERSION_MBEDTLS}" https://github.com/ARMmbed/mbedtls.git
checkResult $? "Line Number: ${LINENO} - Downloading MbedTLS"
sudo make -j4 -C mbedtls install
checkResult $? "Line Number: ${LINENO} - Installing MbedTLS"
rm -fr mbedtls

# install BoringSSL from source
echo
echo "================================================================================"
echo "Installing BoringSSL."
echo "================================================================================"
rm -fr boringssl
# BoringSSL 1.1.0 commit 2a8e294b70f2a5906f717f8a0466d354dca12f25
# BoringSSL 1.0.2 commit 7b8b9c17db93ea5287575b437c77fb36eeb81b31
VERSION_BORINGSSL="2a8e294b70f2a5906f717f8a0466d354dca12f25" # https://github.com/google/boringssl
mkdir boringssl
(
    pushd boringssl > /dev/null
    # Not doing git clone --depth 1 since BoringSSL git repository does not have tags
    # The git fetch method below does a shallow fetch of the specific commit
    git init
    git remote add origin https://github.com/google/boringssl.git
    git fetch --depth 1 origin "${VERSION_BORINGSSL}"
    checkResult $? "Line Number: ${LINENO} - Downloading BoringSSL"
    git checkout FETCH_HEAD
    sed -i -- "s/armv7-a/armv7l/g" CMakeLists.txt
    sed -i "s/-Werror//g" CMakeLists.txt
    cmake -DBUILD_SHARED_LIBS=1 .
    checkResult $? "Line Number: ${LINENO} - Configure BoringSSL"
    make
    checkResult $? "Line Number: ${LINENO} - Compile BoringSSL"
    sudo rm -rf /usr/local/include/boringssl
    sudo rm -rf /usr/local/lib/boringssl
    sudo mkdir /usr/local/include/boringssl
    sudo mkdir /usr/local/lib/boringssl
    sudo cp -r include/* /usr/local/include/boringssl
    sudo cp ssl/libssl.so /usr/local/lib/boringssl
    sudo cp crypto/libcrypto.so /usr/local/lib/boringssl
    sudo cp decrepit/libdecrepit.so /usr/local/lib/boringssl
)
rm -fr boringssl

# install fdk-aac from source
echo
echo "================================================================================"
echo "Installing fdk-aac."
echo "================================================================================"
VERSION_FDKAAC="v2.0.1"
rm -fr fdk-aac
git clone --branch "${VERSION_FDKAAC}" --depth 1 https://github.com/mstorsjo/fdk-aac.git
checkResult $? "Line Number: ${LINENO} - Downloading fdk-aac source"
cd fdk-aac
autoreconf -fiv
./configure --enable-shared=no
checkResult $? "Line Number: ${LINENO} - Configure fdk-aac"
make -j4
checkResult $? "Line Number: ${LINENO} - Build fdk-aac"
sudo make install
checkResult $? "Line Number: ${LINENO} - Install fdk-aac"
cd ..
rm -fr fdk-aac

# install dhcpcd from source
echo
echo "================================================================================"
echo "Installing dhcpcd."
echo "================================================================================"
VERSION_DHCPCD="dhcpcd-6.11.5"
rm -fr dhcpcd
git clone --branch "${VERSION_DHCPCD}" --depth 1 https://github.com/rsmarples/dhcpcd.git
checkResult $? "Line Number: ${LINENO} - Downloading dhcpcd source"
cd dhcpcd
./configure --libexecdir=/lib/dhcpcd --dbdir=/var/lib/dhcpcd5 --rundir=/run
checkResult $? "Line Number: ${LINENO} - Configure dhcpcd"
make -j4
checkResult $? "Line Number: ${LINENO} - Build dhcpcd"
sudo make install
checkResult $? "Line Number: ${LINENO} - Install dhcpcd"
cd ..
rm -fr dhcpcd

# Install libnfc. Dependency for accessory setup programmable NFC tag.
echo
echo "================================================================================"
echo "Installing libnfc."
echo "================================================================================"
rm -fr libnfc-"${VERSION_LIBNFC}"
tar xjf libnfc-"${VERSION_LIBNFC}".tar.bz2
checkResult $? "Line Number: ${LINENO} - Unpacking file"
rm libnfc-"${VERSION_LIBNFC}".tar.bz2
cd libnfc-"${VERSION_LIBNFC}"
./configure --sysconfdir=/etc --prefix=/usr
checkResult $? "Line Number: ${LINENO} - Configuring Lib NFC"
make -j4
checkResult $? "Line Number: ${LINENO} - Building Lib NFC"
sudo make install
checkResult $? "Line Number: ${LINENO} - Installing Lib NFC"
cd ..
rm -fr libnfc-"${VERSION_LIBNFC}"

# Install mDNSResponder. Dependency for Service Discovery.
echo
echo "================================================================================"
echo "Installing mDNSResponder."
echo "================================================================================"
rm -fr mDNSResponder-"${VERSION_MDNS}"
tar xzf mDNSResponder-"${VERSION_MDNS}".tar.gz
mDNSPatch="0001-Fix-mdns-v1310.80.1-compilation.patch"
checkResult $? "Line Number: ${LINENO} - Unzipping mDNSResponder"
git -C mDNSResponder-"${VERSION_MDNS}" apply "../${mDNSPatch}"
rm "${mDNSPatch}"
checkResult $? "Line Number: ${LINENO} - Patching mDNSResponder"
rm mDNSResponder-"${VERSION_MDNS}".tar.gz
mDNSFlags=(
    "os=linux"
)
make -C mDNSResponder-"${VERSION_MDNS}"/mDNSPosix "${mDNSFlags[@]}"
checkResult $? "Line Number: ${LINENO} - Building mDNSResponder"
sudo make -C mDNSResponder-"${VERSION_MDNS}"/mDNSPosix install "${mDNSFlags[@]}"
sudo sed -e '/\bmdns\b/!s/^\(hosts:.*\)dns\(.*\)/\1mdns dns\2/' /etc/nsswitch.conf.pre-mdns | sudo tee /etc/nsswitch.conf > /dev/null
checkResult $? "Line Number: ${LINENO} - Building mDNSResponder"
rm -fr mDNSResponder-"${VERSION_MDNS}"

echo
echo "================================================================================"
echo "Installing libwebsockets"
echo "================================================================================"
rm -fr libwebsockets
git clone --depth 1 https://github.com/warmcat/libwebsockets.git --branch v4.1-stable
cd libwebsockets
mkdir -p build-linux
cd build-linux
cmake .. -DLWS_WITHOUT_EXTENSIONS=1 -DLWS_WITH_SSL=0 -DLWS_WITH_ZLIB=0 -DLWS_WITH_ZIP_FOPS=0 -DLWS_WITH_SERVER_STATUS=1 -DLWS_IPV6=1
checkResult $? "Line Number: ${LINENO} - libwebsockets cmake"
make -j4
checkResult $? "Line Number: ${LINENO} - libwebsockets build"
sudo make install
checkResult $? "Line Number: ${LINENO} - libwebsockets install"
cd ../..
rm -fr libwebsockets

echo
echo "================================================================================"
echo "Creating host-local service."
echo "================================================================================"
cat <<EOF | sudo tee /etc/systemd/system/host-local.service
[Unit]
Description=Hostname at local
After=dhcpcd.service mdns.service

[Service]
ExecStart=/usr/bin/dns-sd -R RASPI _hostname._tcp local 65335
Restart=on-failure
StartLimitIntervalSec=300
StartLimitBurst=100

[Install]
WantedBy=multi-user.target
EOF

echo
echo "================================================================================"
echo "Configuring DHCP."
echo "================================================================================"
sudo perl -i -p0e 's/#option ntp_servers/option ntp_servers/gs' /etc/dhcpcd.conf
checkResult $? "Line Number: ${LINENO} - Configuring DHCP"

echo
echo "================================================================================"
echo "Configuring mdns service."
echo "================================================================================"
cat <<EOF | sudo tee /usr/share/dhcpcd/hooks/99-restart-mdns
restart_mdns()
{
    systemctl restart mdns.service
}

if \\\\\\\$if_up; then
    restart_mdns
fi
EOF

# Configure Raspbian.
echo
echo "================================================================================"
echo "Configuring Raspbian."
echo "================================================================================"
sudo rm -f /etc/systemd/system/dhcpcd.service.d/wait.conf
sudo systemctl enable host-local.service
checkResult $? "Line Number: ${LINENO} - Systemctl Enable"
# Disable the default avahi mdns responder to ensure only mdnsd is running
sudo systemctl mask avahi-daemon.service
checkResult $? "Line Number: ${LINENO} - Systemctl Mask Service"
sudo systemctl mask avahi-daemon.socket
checkResult $? "Line Number: ${LINENO} - Systemctl Mask Socket"
