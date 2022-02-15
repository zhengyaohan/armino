#############################################################################
# Copyright (c) 2014-2019 INSIDE Secure B.V. All Rights Reserved.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
##############################################################################

##############################################################################
##  Makefile for the driver
##  How to use :
##  i)    $ make
##  ii)   For cleaning : $ make clean
##############################################################################

#### User Edit Section : Variables to be set by the User
##   User may not need to modify anything other than this section

  ##-->: { Here follow the variables to be set by the User

	# Set KERNEL_DIR to the path to the Kernel Source directory for which you
	# wish to compile.
	#KERNEL_DIR:=

	# Set ARCH according to the target architecture
	# supported values: x86, arm, arm64 and powerpc
	#ARCH:=x86
	#ARCH:=arm
	#ARCH:=arm64
	#ARCH:=powerpc

	# Set CROSS_COMPILE to the path to the cross compiler binutils
	# concatenated with the prefix they have before normal binutils name
	# For example :
	# If we have cross compiler for Power-PC in /opt/eldk/target/usr/bin/
	# by the file name ppc_4xxFP-gcc, we set it as below :
	#CROSS_COMPILE:=/opt/eldk/target/usr/bin/

	# Set HOST_HW_PLATFORM
	# supported values: FPGA_V2M_Juno, FPGA_Virtex6_PCI and FPGA_Zynq_ZC702
	#HOST_HW_PLATFORM:=FPGA_V2M_Juno
	#HOST_HW_PLATFORM:=FPGA_Virtex6_PCI
	#HOST_HW_PLATFORM:=FPGA_Zynq_ZC702
	FW_EIP130_NAME:=firmware_eip130ram
	FW_EIP130_PATH:=/lib/firmware

  ##<--:  } End of User Edit Section

#### User should not need to modify anything below this line
##############################################################################

## Make sure that ARCH, KERNEL_DIR, HOST_HW_PLATFORM and CROSS_COMPILE are set

ifndef KERNEL_DIR
    ERROR_KERNEL_DIR_NOT_SET:=YES
endif

ifeq (${DDK_HOST64},y)
    EXTRA_CFLAGS += -DDRIVER_64BIT_HOST
endif

ifeq (${DDK_DEVICE64},y)
    EXTRA_CFLAGS += -DDRIVER_64BIT_DEVICE
endif

ifeq (${HOST_HW_PLATFORM},FPGA_Zynq_ZC702)
    EIP130_BUS_TYPE:=OF
else ifeq (${HOST_HW_PLATFORM},FPGA_V2M_Juno)
    EIP130_BUS_TYPE:=OF
else ifeq (${HOST_HW_PLATFORM},FPGA_Virtex6_PCI)
    EIP130_BUS_TYPE:=PCI
else
    ERROR_HOST_HW_PLATFORM_NOT_SET:=YES
endif

ifeq (${ARCH},x86)
    HOST_COMPILER:=gcc
    COMPILER:=gcc
    AR:=ar
    # cs_*.h files respond to ARCH_X86
    EXTRA_CFLAGS += -DARCH_X86
    OBJCOPY = objcopy
    ifeq (${DDK_HOST64},y)
        OBJ_FORMAT = elf64-x86-64
        OBJ_ARCH = i386:x86-64
    else
        OBJ_FORMAT = elf32-i386
        OBJ_ARCH = i386
    endif
else ifeq (${ARCH},powerpc)
    ifndef CROSS_COMPILE
        ERROR_CROSS_COMPILE_NOT_SET:=YES
    endif
    HOST_COMPILER:=gcc
    COMPILER:=${CROSS_COMPILE}gcc
    AR:=${CROSS_COMPILE}ar
    EXTRA_CFLAGS += -DARCH_POWERPC
else ifeq (${ARCH},arm)
    ifndef CROSS_COMPILE
        ERROR_CROSS_COMPILE_NOT_SET:=YES
    endif
    HOST_COMPILER:=gcc
    COMPILER:=${CROSS_COMPILE}gcc
    AR:=${CROSS_COMPILE}ar
    EXTRA_CFLAGS += -DARCH_ARM
else ifeq (${ARCH},arm64)
    ifndef CROSS_COMPILE
        ERROR_CROSS_COMPILE_NOT_SET:=YES
    endif
    HOST_COMPILER:=gcc
    COMPILER:=${CROSS_COMPILE}gcc
    AR:=${CROSS_COMPILE}ar
    EXTRA_CFLAGS += -DARCH_ARM64
else ifeq (${ARCH},mb)
    ifndef CROSS_COMPILE
        ERROR_CROSS_COMPILE_NOT_SET:=YES
    endif
    HOST_COMPILER:=gcc
    COMPILER:=${CROSS_COMPILE}gcc
    AR:=${CROSS_COMPILE}ar
    EXTRA_CFLAGS += -DARCH_MB
    OBJCOPY = ${CROSS_COMPILE}objcopy
    ifneq (,$(findstring little-endian,${USER_CFLAGS}))
        OBJ_FORMAT = elf32-microblazeel
    else
        OBJ_FORMAT = elf32-microblaze
        USER_CFLAGS += -DARCH_MB_BE
    endif
    OBJ_ARCH = MicroBlaze
else
    ERROR_ARCH_NOT_SET:=YES
endif

ifneq (,$(findstring s,${MAKEFLAGS}))
    AR_VERBOSE:=
else
    AR_VERBOSE:=v
endif

ifeq ($(KERNELRELEASE),)
    PWD:=$(shell pwd)
endif

ifeq ($(DDK_PWD_OVERRIDE),)
    DDK_PWD_OVERRIDE := `pwd`
else
    override PWD := $(DDK_PWD_OVERRIDE)
endif

DRIVER_VIP_NAME_KO:=driver_vip_k
EXTRA_CFLAGS += -DDRIVER_VEX_STUB_ENABLE

ifeq ($(DDK_VEX_PROXY),y)
    EXTRA_CFLAGS += -DDRIVER_VEX_PROXY_ENABLE
    DRIVER_VIP_NAME_U:=driver_vip_up
else
    DRIVER_VIP_NAME_U:=driver_vip_u
endif

ifeq (${ARCH},mb)
    USER_FLAGS_WARN=
else
    USER_FLAGS_WARN = -W -Wall -Wextra -Wvla -Wlogical-op \
                      -Wstrict-prototypes -Wmissing-prototypes \
                      -Wshadow -Wnested-externs -Wundef -Wmissing-declarations \
                      -Wold-style-definition -Wpointer-arith -Wdouble-promotion \
                      -Wcast-align -Wcast-qual -Wconversion -Wsign-conversion \
                      -Wjump-misses-init -Wwrite-strings -Wformat=2
ifeq (${ARCH},x86)
    USER_FLAGS_WARN += -Wrestrict -Wduplicated-cond -Wduplicated-branches
endif
    # -Wswitch-enum
endif

#GCOV_PROFILE=y

SRC_APP=../../../Integration
SRC_KIT=../../../Kit
SRC_UMDEVXS_PROXY=../../../Integration/UMDevXS/UserPart
SRC_UMDEVXS_DRV=../../../Integration/UMDevXS/KernelPart
SRC_DRV_FWK=../../../Integration/DriverFramework

PATH_BUILD=$(PWD)
PATH_APP=$(PWD)/$(SRC_APP)
PATH_KIT=$(PWD)/$(SRC_KIT)
PATH_DRV_FWK=$(PWD)/$(SRC_DRV_FWK)
PATH_UMDEVXS_PROXY=$(PWD)/$(SRC_UMDEVXS_PROXY)
PATH_UMDEVXS_DRV=$(PWD)/$(SRC_UMDEVXS_DRV)
PATH_INT_DSP=$(PWD)/$(SRC_INT_DSP)
PATH_WRK_TRD=$(PWD)/$(SRC_WRK_TRD)

# Include Paths
INCLUDE_APP_CONFIG:= \
  -I$(PATH_BUILD) \
  -I$(PATH_BUILD)/$(HOST_HW_PLATFORM)

INCLUDE_APP:= \
  -I$(PATH_APP)/Adapter_DriverInit/src \
  -I$(PATH_APP)/Adapter_DriverInit/incl \
  -I$(PATH_APP)/Adapter_DriverInit/src/$(HOST_HW_PLATFORM) \
  -I$(PATH_APP)/Adapter_DriverInit/src/lkm \
  -I$(PATH_APP)/Adapter_GlobalControl/src \
  -I$(PATH_APP)/Adapter_GlobalControl/incl \
  -I$(PATH_APP)/Adapter_Generic/src \
  -I$(PATH_APP)/Adapter_Generic/src/$(HOST_HW_PLATFORM) \
  -I$(PATH_APP)/Adapter_Generic/src/lkm \
  -I$(PATH_APP)/Adapter_VAL/src \
  -I$(PATH_APP)/Adapter_VAL/src/lkm \
  -I$(PATH_APP)/Adapter_VAL/incl \
  -I$(PATH_APP)/Adapter_VEX/src \
  -I$(PATH_APP)/VEX_Proxy_Stub/KernelPart/incl

INCLUDE_KIT:= \
  -I$(PATH_KIT)/DriverFramework/Basic_Defs_API/incl \
  -I$(PATH_KIT)/DriverFramework/CLib_Abstraction_API/incl \
  -I$(PATH_KIT)/DriverFramework/Device_API/incl \
  -I$(PATH_KIT)/DriverFramework/DMAResource_API/incl \
  -I$(PATH_UMDEVXS_DRV)/incl \
  -I$(PATH_DRV_FWK)/src/LKM \
  -I$(PATH_DRV_FWK)/src/LKM/$(EIP130_BUS_TYPE) \
  -I$(PATH_DRV_FWK)/src \
  -I$(PATH_KIT)/EIP130/TokenHelper/incl \
  -I$(PATH_KIT)/EIP130/TokenHelper/src \
  -I$(PATH_KIT)/EIP130/MailboxControl/incl \
  -I$(PATH_KIT)/EIP130/MailboxControl/src \
  -I$(PATH_KIT)/Log/incl \
  -I$(PATH_KIT)/Log/src/printk

ifeq (${ARCH},mb)
    ADAPTER_DPLM = mb_freertos
    INCLUDE_USER_LOG:=-I$(PATH_KIT)/Log/src/xilprintf
    INCLUDE_USER_DRW_FWK:=-I$(PATH_DRV_FWK)/src/mb_freertos
else
    ADAPTER_DPLM = um
    INCLUDE_USER_LOG:=-I$(PATH_KIT)/Log/src/printf
    INCLUDE_USER_DRW_FWK:=-I$(PATH_DRV_FWK)/src/UMDevXS
endif

INCLUDE_USER:= \
  -I$(PATH_APP)/Adapter_Generic/src/$(ADAPTER_DPLM) \
  -I$(PATH_APP)/Adapter_DriverInit/src/um \
  -I$(PATH_APP)/Adapter_VAL/src/$(ADAPTER_DPLM) \
  -I$(PATH_APP)/VEX_Proxy_Stub/UserPart/src \
  $(INCLUDE_USER_DRW_FWK) \
  $(INCLUDE_USER_LOG) \
  -I$(PATH_UMDEVXS_PROXY)/incl \
  -I$(PATH_UMDEVXS_DRV)/incl

INCLUDE_FLAGS_ALL:= \
  $(INCLUDE_APP_CONFIG) \
  $(INCLUDE_APP) \
  $(INCLUDE_KIT)

ifeq (${ARCH},powerpc)
    INCLUDE_FLAGS_ALL += -I$(strip $(KERNEL_DIR))/arch/powerpc
    INCLUDE_FLAGS_ALL += -I$(strip $(KERNEL_DIR))/arch/powerpc/include
    INCLUDE_FLAGS_ALL += -I$(strip $(KERNEL_DIR))/include
endif

# Compiler Flags
WARNING_FLAGS += -Wall
#DEBUG_FLAGS=
#CODE_COVERAGE_FLAGS=
#PROFILING_FLAGS=
#BACKWARD_COMPATIBILITY_FLAGS=
#OPTIMIZATION_FLAGS=

ifeq (${DEBUG_DDK},y)
    DEBUG_FLAGS += -g
    DEBUG_FLAGS += -DDEBUG
endif

ifeq (${COVER_DDK},y)
    DEBUG_FLAGS += -g
    USER_CFLAGS += -ftest-coverage
    USER_CFLAGS += -fprofile-arcs
endif

ifeq (${PROFILE_DDK},y)
    DEBUG_FLAGS += -g
    USER_CFLAGS += -pg
endif

# ARCH=powerpc means build for 405EX which has 16K L1 I-Cache
ifeq (${ARCH},powerpc)
    #OPTIMIZATION_FLAGS += -Os
endif

EXTRA_CFLAGS += $(BACKWARD_COMPATIBILITY_FLAGS)
EXTRA_CFLAGS += $(INCLUDE_FLAGS_ALL)
EXTRA_CFLAGS += $(WARNING_FLAGS)
EXTRA_CFLAGS += $(DEBUG_FLAGS)
EXTRA_CFLAGS += $(CODE_COVERAGE_FLAGS)
EXTRA_CFLAGS += $(PROFILING_FLAGS)
EXTRA_CFLAGS += $(OPTIMIZATION_FLAGS)

USER_CFLAGS += $(USER_FLAGS_WARN) $(INCLUDE_USER)

.PHONY = clean clean_module

ifeq (${ARCH},mb)
    DRIVER_BUILD_RULES = firmware_eip130 driver_vip_u
else
    DRIVER_BUILD_RULES = $(DRIVER_VIP_NAME_KO) driver_vip_u
endif

default: $(DRIVER_BUILD_RULES)

# Linux kernel-space driver build
obj-m:= $(DRIVER_VIP_NAME_KO).o

vex_objs:= $(patsubst %c,%o,$(subst $(PWD)/,,$(wildcard $(PATH_APP)/Adapter_VEX/src/*.c)))
val_objs:= $(patsubst %c,%o,$(subst $(PWD)/,,$(wildcard $(PATH_APP)/Adapter_VAL/src/*.c)))
adapter_global_objs:= $(patsubst %c,%o,$(subst $(PWD)/,,$(wildcard $(PATH_APP)/Adapter_Global/src/*.c)))
adapter_gen_objs:= $(patsubst %c,%o,$(subst $(PWD)/,,$(wildcard $(PATH_APP)/Adapter_Generic/src/*.c)))
adapter_gen_lkm_objs:= $(patsubst %c,%o,$(subst $(PWD)/,,$(wildcard $(PATH_APP)/Adapter_Generic/src/lkm/*.c)))
drvlib_objs:= $(patsubst %c,%o,$(subst $(PWD)/,,$(wildcard $(PATH_KIT)/EIP130/*/src/*.c)))
vex_stub_objs:= $(SRC_APP)/UMDevXS/KernelPart/src/umdevxs_chrdev.o \
                $(SRC_APP)/VEX_Proxy_Stub/KernelPart/src/vex_stub.o

# relative paths required here!
$(DRIVER_VIP_NAME_KO)-objs:= \
  $(drvlib_objs) \
  $(SRC_KIT)/Log/src/log.o \
  $(SRC_APP)/DriverFramework/src/device_generic.o \
  $(SRC_APP)/DriverFramework/src/LKM/$(EIP130_BUS_TYPE)/device_lkm.o \
  $(SRC_APP)/DriverFramework/src/LKM/$(EIP130_BUS_TYPE)/lkm.o \
  $(SRC_APP)/DriverFramework/src/LKM/dmares_lkm.o \
  $(SRC_APP)/DriverFramework/src/dmares_gen.o \
  $(vex_objs) \
  $(val_objs) \
  $(adapter_global_objs) \
  $(adapter_gen_objs) \
  $(adapter_gen_lkm_objs) \
  $(SRC_APP)/Adapter_DriverInit/src/$(HOST_HW_PLATFORM)/adapter_init.o \
  $(SRC_APP)/Adapter_DriverInit/src/adapter_driver_init.o \
  $(vex_stub_objs)

# Linux kernel-space driver build
$(DRIVER_VIP_NAME_KO):
	@if [ "$(ERROR_ARCH_NOT_SET)" = "YES" ]; then \
	    echo "ARCH not set or unsupported"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_KERNEL_DIR_NOT_SET)" = "YES" ]; then \
	    echo "KERNEL_DIR not set"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_CROSS_COMPILE_NOT_SET)" = "YES" ]; then \
	    echo "CROSS_COMPILE not set"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_HOST_HW_PLATFORM_NOT_SET)" = "YES" ]; then \
	    echo "HOST_HW_PLATFORM not set"; \
	    exit 1; \
	fi
#	@echo "EXTRA_CFLAGS: $(EXTRA_CFLAGS)";
	make DDK_PWD_OVERRIDE=$(DDK_PWD_OVERRIDE) $(EXTRA_KBUILD_FLAGS) ARCH=$(ARCH) CROSS_COMPILE=$(strip $(CROSS_COMPILE)) -C $(KERNEL_DIR) SUBDIRS=$(PWD) M=$(PWD) modules

firmware_eip130:
	$(OBJCOPY) -I binary -O $(OBJ_FORMAT) -B $(OBJ_ARCH) $(FW_EIP130_PATH)/$(FW_EIP130_NAME).sbi $(FW_EIP130_NAME).o

# Linux user-space driver build
driver_vip_u:
	@if [ "$(ERROR_ARCH_NOT_SET)" = "YES" ]; then \
	    echo "ARCH not set or unsupported"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_CROSS_COMPILE_NOT_SET)" = "YES" ]; then \
	    echo "CROSS_COMPILE not set"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_HOST_HW_PLATFORM_NOT_SET)" = "YES" ]; then \
	    echo "HOST_HW_PLATFORM not set"; \
	    exit 1; \
	fi
# Adapter Generic
	$(COMPILER) -c $(SRC_APP)/Adapter_Generic/src/*.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
	$(COMPILER) -c $(SRC_APP)/Adapter_Generic/src/$(ADAPTER_DPLM)/*.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
# Adapter Global Control
	$(COMPILER) -c $(SRC_APP)/Adapter_GlobalControl/src/*.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
# Adapter Driver Init
	$(COMPILER) -c $(SRC_APP)/Adapter_DriverInit/src/$(HOST_HW_PLATFORM)/adapter_init.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
	$(COMPILER) -c $(SRC_APP)/Adapter_DriverInit/src/adapter_driver_init.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
# Adapter VAL
	$(COMPILER) -c $(SRC_APP)/Adapter_VAL/src/*.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
# Adapter VEX (including adapter_vex_LogicalToken.o with vex_LogicalToken())
	$(COMPILER) -c $(SRC_APP)/Adapter_VEX/src/*.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
# Log
	$(COMPILER) -c $(SRC_KIT)/Log/src/log.c  $(USER_CFLAGS) $(EXTRA_CFLAGS)
# UMDevXS Proxy
	@if [ "${ARCH}" != "mb" ] ; then \
	    $(COMPILER) -c $(SRC_UMDEVXS_PROXY)/src/umdevxsproxy.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	fi
# VEX Proxy (excluding adapter_vex_LogicalToken.o with vex_LogicalToken()) or
# EIP-130 Driver Library with user-space Driver Framework modules and library
	@if [ "${DRIVER_VIP_NAME_U}" = "driver_vip_up" ] ; then \
	    $(COMPILER) -c $(SRC_APP)/VEX_Proxy_Stub/UserPart/src/*.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	    rm -f adapter_vex_logicaltoken.o eip130*.o dmares_gen.o hwpal_*.o ; \
	else \
	    $(COMPILER) -c $(SRC_KIT)/EIP130/*/src/*.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	    $(COMPILER) -c $(SRC_DRV_FWK)/src/device_generic.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	    $(COMPILER) -c $(SRC_DRV_FWK)/src/dmares_gen.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	    if [ "${ARCH}" = "mb" ] ; then \
	        $(COMPILER) -c $(SRC_DRV_FWK)/src/mb_freertos/hwpal_dmares_mb.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	        $(COMPILER) -c $(SRC_DRV_FWK)/src/mb_freertos/hwpal_device_mb.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	        $(AR) -cq$(AR_VERBOSE) libdf.a log.o device_generic.o hwpal_device_mb.o; \
	    else \
	        $(COMPILER) -c $(SRC_DRV_FWK)/src/UMDevXS/hwpal_dmares_umdevxs.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	        $(COMPILER) -c $(SRC_DRV_FWK)/src/UMDevXS/hwpal_device_umdevxs.c  $(USER_CFLAGS) $(EXTRA_CFLAGS); \
	        $(AR) -cq$(AR_VERBOSE) libdf.a log.o device_generic.o hwpal_device_umdevxs.o umdevxsproxy.o; \
	    fi \
	fi
# Driver user-space statically linkable library
	rm -f lib$(DRIVER_VIP_NAME_U).a
	rm -f $(DRIVER_VIP_NAME_KO).o
	ls *.o |grep -v "_k\.o"|grep -v "\.mod\.o"|xargs $(AR) -cq$(AR_VERBOSE) lib$(DRIVER_VIP_NAME_U).a

# Cleaning
clean:	clean_module

clean_module:
	# this is run in $PATH_BUILD where the Makefile resides
	find $(PATH_APP) \( -name "*.o" -o -name ".*.cmd" \) -type f -print | xargs rm -f
	find $(PATH_KIT) \( -name "*.o" -o -name ".*.cmd" \) -type f -print | xargs rm -f
	rm -rf .tmp_versions
	rm -f *.o  *.a .*.cmd  .*.flags  *.mod.c  *.ko  *.symvers  *.markers
	rm -f *.order  *.gcno  *.gcda  *.out


# end of file
