# Install script for directory: /home/jenkins/workspace/Armino_SDK_Push/armino

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/opt/risc-v/nds32le-elf-mculib-v5/bin/riscv32-elf-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk7235/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/common/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/riscv/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_rtos/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_ble/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_ate/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/base64/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/easy_flash/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_libs/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_init/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/arm9/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_usb/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/music_player/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_ps/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/driver/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_event/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_log/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/release/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_startup/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/os_source/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/lwip_intf_v2_0/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/lwip_intf_v2_1/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_system/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_common/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/include/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_netif/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_wifi/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/utf8/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/temp_detect/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/user_driver/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/security/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/demos/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/fatfs/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/http/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/video/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/at/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/compal/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/homekit/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_cli/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/at_server/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/app/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/audio/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/bk_adapter/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/key_handle/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/mbedtls/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/paho-mqtt/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/saradc_intf/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/sim_uart/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/udisk_mp3/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/usb_plug/cmake_install.cmake")
  include("/home/jenkins/workspace/Armino_SDK_Push/armino/build/bk7235/armino/main/cmake_install.cmake")

endif()
