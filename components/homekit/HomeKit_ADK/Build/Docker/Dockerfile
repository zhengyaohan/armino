# Build stage
FROM ubuntu:18.04

RUN echo ========== Install dependencies ========== \
  && apt-get update && apt-get install -y \
    clang \
    gdb \
    git \
    make \
    cmake \
    curl \
    perl \
    unzip \
    wget \
    rsync \
    unifdef \
    libasound2-dev \
    libblocksruntime-dev \
    libfdk-aac-dev \
    libopus-dev \
    libsqlite3-dev \
    openssh-server \
    libnfc-dev \
    golang \
    vim \
    autoconf \
    autotools-dev \
    automake \
    libtool \
    ccache \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
  && rm -rf /var/lib/apt/lists/* \
# Leaving the cross compiler libraries cause incompatibility with sysroot
# libraries even if --sysroot is specified. It might be a bug with the compiler.
# Hence, the library is moved.
  && mv /usr/arm-linux-gnueabihf/lib /usr/arm-linux-gnueabihf/orig-lib

WORKDIR /usr/share
RUN echo ========== Install OpenSSL 1.0 ========== \
  && git clone --recursive --depth 1 --branch OpenSSL_1_0_2n https://github.com/openssl/openssl.git \
  && cd openssl && rm -fr crypto/idea crypto/mdc2 crypto/rc5 \
  && ./config no-idea no-mdc2 no-rc5 shared --prefix=/usr --openssldir=/etc/ssl --libdir=lib/x86_64-linux-gnu \
  && make depend && make && make install && cd .. \
  && rm -rf openssl

WORKDIR /usr/share
RUN echo ========== Install OpenSSL 1.1 ========== \
  && git clone --recursive --depth 1 --branch OpenSSL_1_1_1 https://github.com/openssl/openssl.git \
  && cd openssl && rm -fr crypto/idea crypto/mdc2 crypto/rc5 \
  && ./config no-idea no-mdc2 no-rc5 --prefix=/usr --openssldir=/etc/ssl --libdir=/usr/lib/x86_64-linux-gnu \
  && make && make install && cd .. \
  && rm -rf openssl

WORKDIR /usr/local
RUN echo ========== Install Arm compiler ========== \
  && wget --show-progress -qO- https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2 | tar -xjf -

WORKDIR /usr/share/nRF5-SDK
RUN echo ========== Install Nordic SDK ========== \
  && wget --show-progress -qO nRF5-SDK.zip https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5-SDK-for-Thread/nRF5-SDK-for-Thread-and-Zigbee/nRF5SDKforThreadv41.zip \
  && unzip nRF5-SDK.zip \
  && rm nRF5-SDK.zip

WORKDIR /usr/share
RUN echo ========== Patch openthread ========== \
  && git clone https://github.com/openthread/openthread.git \
  && cd openthread \
  && git checkout 696d6389e681cbd5b1177d92fe981e8ac4e07c89 \
  && PATH=$PATH:/usr/local/gcc-arm-none-eabi-8-2018-q4-major/bin ./bootstrap \
  && PATH=$PATH:/usr/local/gcc-arm-none-eabi-8-2018-q4-major/bin \
     BORDER_AGENT=1 \
     BORDER_ROUTER=1 \
     COAP=1 \
     COMMISSIONER=1 \
     DISABLE_BUILTIN_MBEDTLS=1 \
     DNS_CLIENT=1 \
     DIAGNOSTIC=1 \
     EXTERNAL_HEAP=1 \
     IP6_FRAGM=1 \
     JOINER=1 \
     LINK_RAW=1 \
     MAC_FILTER=1 \
     MTD_NETDIAG=1 \
     SERVICE=1 \
     UDP_FORWARD=1 \
     ECDSA=1 \
     SNTP_CLIENT=1 \
     COAPS=1 \
     DHCP6_SERVER=1 \
     DHCP6_CLIENT=1 \
     CHILD_SUPERVISION=1 \
     JAM_DETECTION=1 \
     make -f examples/Makefile-nrf52840 \
  && cp -fr output/nrf52840/lib/* ../nRF5-SDK/external/openthread/lib/nrf52840/gcc \
  && cp -fr include/openthread/*.h ../nRF5-SDK/external/openthread/include/openthread \
  && cp -fr include/openthread/platform/*.h ../nRF5-SDK/external/openthread/include/openthread/platform \
  && cp -fr src/core/config/*.h ../nRF5-SDK/external/openthread/include/config

WORKDIR /usr/local
COPY Patches/mDNSResponder/0001-Fix-mdns-v1310.80.1-compilation.patch .
ARG FLAGS_MDNS=""
ARG PATCH_MDNS="0001-Fix-mdns-v1310.80.1-compilation.patch"
RUN echo ========== Install mdns responder ========== \
    && VERSION_MDNS="1310.80.1" \
    && wget --show-progress -qO mDNSResponder-"${VERSION_MDNS}".tar.gz https://opensource.apple.com/tarballs/mDNSResponder/mDNSResponder-"${VERSION_MDNS}".tar.gz \
    && tar xzf mDNSResponder-"${VERSION_MDNS}".tar.gz --no-same-owner \
    && if [ -n "${PATCH_MDNS}" ]; then git -C mDNSResponder-"${VERSION_MDNS}" apply "../${PATCH_MDNS}"; fi \
    && if [ -n "${PATCH_MDNS}" ]; then rm "${PATCH_MDNS}"; fi \
    && make -C mDNSResponder-"${VERSION_MDNS}"/mDNSPosix os=linux ${FLAGS_MDNS} \
    && make -C mDNSResponder-"${VERSION_MDNS}"/mDNSPosix install os=linux ${FLAGS_MDNS} \
    && sed -e '/\bmdns\b/!s/^\(hosts:.*\)dns\(.*\)/\1mdns dns\2/' /etc/nsswitch.conf.pre-mdns > /etc/nsswitch.conf \
    && rm mDNSResponder-"${VERSION_MDNS}".tar.gz \
    && rm -fr mDNSResponder-"${VERSION_MDNS}"

# BoringSSL 1.1.0 commit 2a8e294b70f2a5906f717f8a0466d354dca12f25
# BoringSSL 1.0.2 commit 7b8b9c17db93ea5287575b437c77fb36eeb81b31
# Not doing git clone --depth 1 since BoringSSL git repository does not have tags
# The git fetch method below does a shallow fetch of the specific commit
WORKDIR /usr/local
RUN echo ========== Install BoringSSL ========== \
  && rm -fr boringssl \
  && VERSION_BORINGSSL="2a8e294b70f2a5906f717f8a0466d354dca12f25" \
  && mkdir boringssl \
  && cd boringssl \
  && git init \
  && git remote add origin https://github.com/google/boringssl.git \
  && git fetch --depth 1 origin "${VERSION_BORINGSSL}" \
  && git checkout FETCH_HEAD \
  && sed -i "s/-Werror//g" CMakeLists.txt \
  && cmake -DBUILD_SHARED_LIBS=1 . \
  && make \
  && rm -rf /usr/local/include/boringssl \
  && rm -rf /usr/local/lib/boringssl \
  && mkdir /usr/local/include/boringssl \
  && mkdir /usr/local/lib/boringssl \
  && cp -r include/* /usr/local/include/boringssl \
  && cp ssl/libssl.so /usr/local/lib/boringssl \
  && cp crypto/libcrypto.so /usr/local/lib/boringssl \
  && cp decrepit/libdecrepit.so /usr/local/lib/boringssl \
  && cd .. \
  && rm -fr boringssl

RUN echo ========== Install MbedTLS ========== \
  && git clone --recursive --depth 1 --branch mbedtls-2.18.1 https://github.com/ARMmbed/mbedtls.git \
  && make -C mbedtls SHARED=1 install \
  && rm -rf mbedtls

WORKDIR /usr/local
RUN echo ========== Install libwebsockets responder ========== \
  && git clone --depth 1 https://github.com/warmcat/libwebsockets.git --branch v4.1-stable \
  && cd libwebsockets \
  && mkdir -p build-linux \
  && cd build-linux \
  && cmake .. -DLWS_WITHOUT_EXTENSIONS=1 -DLWS_WITH_SSL=0 -DLWS_WITH_ZLIB=0 -DLWS_WITH_ZIP_FOPS=0 -DLWS_WITH_SERVER_STATUS=1 -DLWS_IPV6=1 \
  && make -j4 \
  && make install \
  && cd ../.. \
  && rm -rf libwebsockets

WORKDIR /usr/local
RUN echo ========== Install libcoap ========== \
  && git clone https://github.com/obgm/libcoap.git --branch v4.2.1 --single-branch \
  && cd libcoap \
  && ./autogen.sh \
  && ./configure --disable-dtls --disable-doxygen --disable-manpages \
  && make \
  && make install \
  && cd .. \
  && rm -rf libcoap


WORKDIR /build
