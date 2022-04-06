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
#
##############################################################################

##############################################################################
##  Makefile for the Linux Kernel Support Driver (UMDevXS only)
##  How to use :
##  i)    $ make
##  ii)   For cleaning : $ make clean
##############################################################################

#### User Edit Section : Variables to be set by the User
##   User may not need to modify anything other than this section

  ##-->: { Here follow the variables to be set by the User

	# Set ARCH according to the target architecture
	# supported values: x86, arm, arm64 and powerpc
	#ARCH:=x86
	#ARCH:=arm
	#ARCH:=arm64
	#ARCH:=powerpc

	# Set KERNEL_DIR to the path to the Kernel Source directory for which you
	# wish to compile.
	#KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

	# Set HOST_HW_PLATFORM
	# supported values: FPGA_V2M_Juno, FPGA_Virtex6_PCI and FPGA_Zynq_ZC702
	#HOST_HW_PLATFORM:=FPGA_V2M_Juno
	#HOST_HW_PLATFORM:=FPGA_Virtex6_PCI
	#HOST_HW_PLATFORM:=FPGA_Zynq_ZC702

  ##<--:  } End of User Edit Section

#### User should not need to modify anything below this line
##############################################################################

## Make sure that ARCH, KERNEL_DIR, HOST_HW_PLATFORM and CROSS_COMPILE are set

ifndef KERNEL_DIR
  ERROR_KERNEL_DIR_NOT_SET:=YES
endif

ifeq (${DDK_HOST64},y)
  EXTRA_CFLAGS += -DUMDEVXS_64BIT_HOST
endif

ifeq (${DDK_DEVICE64},y)
  EXTRA_CFLAGS += -DUMDEVXS_64BIT_DEVICE
endif

ifeq (${HOST_HW_PLATFORM},FPGA_V2M_Juno)
  EIP130_BUS_TYPE:=OF
else ifeq (${HOST_HW_PLATFORM},FPGA_Zynq_ZC702)
  EIP130_BUS_TYPE:=OF
else ifeq (${HOST_HW_PLATFORM},FPGA_Zynq_ZC706)
  EIP130_BUS_TYPE:=OF
else ifeq (${HOST_HW_PLATFORM},FPGA_Virtex6_PCI)
  EIP130_BUS_TYPE:=PCI
else ifeq (${HOST_HW_PLATFORM},FPGA_Virtex7_PCI)
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

PATH_BASE=$(PWD)/../../../..
UMDEVXS_BASE=../../../../Integration

# Include Paths
INCLUDE_ALL:= \
    -I$(PWD) \
    -I$(PWD)/$(HOST_HW_PLATFORM) \
    -I$(PATH_BASE)/Kit/DriverFramework/Basic_Defs_API/incl \
    -I$(PATH_BASE)/Kit/DriverFramework/CLib_Abstraction_API/incl \
    -I$(PATH_BASE)/Kit/Log/incl \
    -I$(PATH_BASE)/Kit/Log/src/printk \
    -I$(PATH_BASE)/Integration/DriverFramework/src/LKM \
    -I$(PATH_BASE)/Integration/DriverFramework/src/LKM/$(EIP130_BUS_TYPE) \
    -I$(PATH_BASE)/Integration/UMDevXS/KernelPart/incl \
    -I$(PATH_BASE)/Integration/UMDevXS/KernelPart/src

# Compiler Flags
WARNING_FLAGS=-Wall
ifeq (${ARCH},x86)
    WARNING_FLAGS += -Wextra -Wvla -Wlogical-op -Wrestrict \
                     -Wstrict-prototypes -Wmissing-prototypes \
                     -Wundef -Wmissing-declarations -Wdouble-promotion \
                     -Wjump-misses-init -Wwrite-strings -Wformat=2 \
                     -Wcast-align -Wduplicated-cond -Wduplicated-branches \
                     -Wno-unused-parameter -Wno-discarded-qualifiers
endif
# supress kernel header files issues
# -Wshadow -Wnested-externs -Wcast-qual -Wpointer-arith -Wconversion -Wsign-conversion -Wswitch-enum

#DEBUG_FLAGS=-g

EXTRA_CFLAGS += \
	$(INCLUDE_ALL) \
	$(WARNING_FLAGS) \
	$(DEBUG_FLAGS)

.PHONY = clean

general_k-objs:= \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_lkm.o \
    $(UMDEVXS_BASE)/DriverFramework/src/LKM/$(EIP130_BUS_TYPE)/lkm.o \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_chrdev.o \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_device.o \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_smbuf.o \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_bufadmin.o \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_interrupt.o \
    $(UMDEVXS_BASE)/../Kit/Log/src/log.o

ifeq (${EIP130_BUS_TYPE},OF)
  obj-m:=umdevxs_k.o
  umdevxs_k-objs:= \
    $(general_k-objs) \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_ofdev.o
else ifeq (${EIP130_BUS_TYPE},PCI)
  obj-m:=umpci_k.o
  umpci_k-objs:= \
    $(general_k-objs) \
    $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/umdevxs_pcidev.o
endif

default: build

build:
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

clean:	clean_module

clean_module:
	-rm -rf .tmp_versions *.symvers *.order *.markers *.o .*.cmd .*.flags *.mod.c *.ko
	-rm -f $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/*.o $(UMDEVXS_BASE)/UMDevXS/KernelPart/src/.*.cmd
	-rm -f $(UMDEVXS_BASE)/DriverFramework/src/LKM/$(EIP130_BUS_TYPE)/*.o $(UMDEVXS_BASE)/DriverFramework/src/LKM/$(EIP130_BUS_TYPE)/.*.cmd
	-rm -f $(UMDEVXS_BASE)/../Kit/Log/src/*.o $(UMDEVXS_BASE)/../Kit/Log/src/.*.cmd

# end of file
