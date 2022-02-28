#############################################################################
# Copyright (c) 2017 INSIDE Secure B.V. All Rights Reserved.
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
##  Makefile for Test Tool for VAL API
##  How to use :
##  i)    $ make
##  ii)   For cleaning : $ make clean
##############################################################################

#### User Edit Section : Variables to be set by the User
##   User may not need to modify anything other than this section

  ##-->: { These variables in this section are set by the user.

	# Set ARCH according to the target architecture
	# supported values: mb
	#ARCH:=mb

	# Set PATH_DRIVER_VIP_SYMVERS to the Driver VIP build directory where
	# the driver static library file is located. If not set then a default build
	# location will be used.

	# Set CROSS_COMPILE to the path to the cross compiler binutils
	# concatenated with the prefix they have before normal binutils name
	# For example :
	# If we have cross compiler for Power-PC in /opt/eldk/target/usr/bin/
	# by the file name ppc_4xxFP-gcc, we set it as below :
	#CROSS_COMPILE:=/opt/eldk/target/usr/bin/

	# Set CROSS_LIB_PATH to the path to the cross compiler libraries
	#CROSS_LIB_PATH:=/opt/eldk/ppc_4xxFP/lib

	# Set HOST_HW_PLATFORM
	# supported values: FPGA_Zynq_ZC702
	#HOST_HW_PLATFORM:=FPGA_Zynq_ZC702

  ##<--:  } End of User Edit Section

#### User should not need to modify anything below this line
##############################################################################

## Make sure that ARCH, HOST_HW_PLATFORM and CROSS_COMPILE are set

ifndef HOST_HW_PLATFORM
    ERROR_HOST_HW_PLATFORM_NOT_SET:=YES
endif

ifndef CROSS_LIB_PATH
    ERROR_CROSS_LIB_PATH_NOT_SET:=YES
endif

ifndef REGRESS_LIB_PATH
    ERROR_REGRESS_LIB_PATH_NOT_SET:=YES
endif

ifndef REGRESS_LIB
    ERROR_REGRESS_LIB_NOT_SET:=YES
endif

# cs_*.h files respond to ARCH_MB
ifeq (${ARCH},mb)
    ifndef CROSS_COMPILE
        ERROR_CROSS_COMPILE_NOT_SET:=YES
    endif
    HOST_COMPILER:=gcc
    COMPILER:=${CROSS_COMPILE}gcc
    AR:=${CROSS_COMPILE}ar
    EXTRA_CFLAGS += -DARCH_MB
    LIB_PATH:=${CROSS_LIB_PATH}
else
    ERROR_ARCH_NOT_SET:=YES
endif

ifneq (,$(findstring s,${MAKEFLAGS}))
    AR_VERBOSE:=
else
    AR_VERBOSE:=v
endif

ifeq ($(DDK_PWD_OVERRIDE),)
    DDK_PWD_OVERRIDE := `pwd`
else
    override PWD := $(DDK_PWD_OVERRIDE)
endif

REGRESS_NAME:=regress
DRIVER_VIP_NAME_U:=driver_vip_u

DRIVERLIB_VIP=-l$(DRIVER_VIP_NAME_U)

ifeq ($(PATH_DRIVER_VIP_SYMVERS),)
    PATH_DRIVER_VIP_SYMVERS=../../../Examples/Driver/build
endif

PATH_BASE=$(PWD)/..

# Include Paths
INCLUDE_KIT:= \
  -I$(PATH_BASE)/../../Kit/DriverFramework/Basic_Defs_API/incl \
  -I$(PATH_BASE)/../../Kit/DriverFramework/CLib_Abstraction_API/incl \
  -I$(PATH_BASE)/../../Kit/Log/incl \

INCLUDE_USER:=-I$(PATH_BASE)/$(PATH_KIT)/Log/src/xilprintf

INCLUDE_ALL:= \
  $(INCLUDE_KIT) \
  -I$(PATH_BASE)/../../Examples/DemoApp_EncryptVector/src \
  -I$(PATH_BASE)/../../Examples/DemoApp_OTPKeyBlob/src \
  -I$(PATH_BASE)/../../Examples/DemoApp_OTPProgram/src \
  -I$(PATH_BASE)/../../Examples/DemoApp_SecureDebug/src \
  -I$(PATH_BASE)/../../Examples/DemoApp_VAL/src \
  -I$(PATH_BASE)/../../Examples/DemoApp_VAL_Specials/src \
  -I$(PATH_BASE)/../../TestTools/TestTool_VAL/src

# Compiler Flags
WARNING_FLAGS += -Wall
#DEBUG_FLAGS=
#CODE_COVERAGE_FLAGS=
#PROFILING_FLAGS=
#BACKWARD_COMPATIBILITY_FLAGS=
EMPTY:=
SPACE:= $(EMPTY) $(EMPTY)
UNDER:=_
CONFIG_FLAGS=-D$(word 1, $(subst $(UNDER),$(SPACE),$(HOST_HW_PLATFORM)))

ifeq (${DEBUG_DDK},y)
    DEBUG_FLAGS += -g
    DEBUG_FLAGS += -DDEBUG
endif

ifeq (${COVER_DDK},y)
    DEBUG_FLAGS += -g
    USER_CFLAGS += -ftest-coverage
    USER_CFLAGS += -fprofile-arcs
    USER_LFLAGS += -lgcov
endif

ifeq (${PROFILE_DDK},y)
    USER_CFLAGS += -pg
    USER_LFLAGS += -pg
endif

EXTRA_CFLAGS += \
  $(BACKWARD_COMPATIBILITY_FLAGS) \
  $(INCLUDE_ALL) \
  $(INCLUDE_KERNEL) \
  $(WARNING_FLAGS) \
  $(DEBUG_FLAGS) \
  $(CODE_COVERAGE_FLAGS) \
  $(PROFILING_FLAGS) \
  $(CONFIG_FLAGS)
# above INCLUDE_KERNEL will also be present for user-mode builds
# we conquer that by ensuring the user-mode include is first in the path

USER_CFLAGS += \
  $(INCLUDE_USER)

.PHONY = clean clean_module combined

combined: default

default: mbfreertos $(REGRESS_NAME) boot

mbfreertos:
	$(COMPILER) -c mbfreertos.c $(USER_CFLAGS) $(EXTRA_CFLAGS)
	$(AR) -cq libmbfreertos.a mbfreertos.o

$(REGRESS_NAME):
	@if [ "$(ERROR_ARCH_NOT_SET)" = "YES" ]; then \
	    echo "ARCH not set or unsupported"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_CROSS_COMPILE_NOT_SET)" = "YES" ]; then \
	    echo "CROSS_COMPILE not set"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_CROSS_LIB_PATH_NOT_SET)" = "YES" ]; then \
	    echo "CROSS_LIB_PATH not set"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_HOST_HW_PLATFORM_NOT_SET)" = "YES" ]; then \
	    echo "HOST_HW_PLATFORM not set"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_REGRESS_LIB_PATH_NOT_SET)" = "YES" ]; then \
	    echo "REGRESS_LIB_PATH not set"; \
	    exit 1; \
	fi
	@if [ "$(ERROR_REGRESS_LIB_NOT_SET)" = "YES" ]; then \
	    echo "REGRESS_LIB not set"; \
	    exit 1; \
	fi
	$(COMPILER) -c $(REGRESS_NAME).c $(USER_CFLAGS) $(EXTRA_CFLAGS)
	$(COMPILER) -o $(REGRESS_NAME).elf $(REGRESS_NAME).o \
	        -L$(LIB_PATH) -L$(PATH_DRIVER_VIP_SYMVERS) $(REGRESS_LIB_PATH) \
	        $(REGRESS_LIB) $(DRIVERLIB_VIP) \
	        $(EARLY_USER_LFLAGS) $(USER_LFLAGS)

boot:
	bootgen -image fsbl.bif -arch zynq -o BOOT.bin -w on

# Cleaning
clean:	clean_module

clean_module:
	rm -rf .tmp_versions *.o .*.cmd .*.flags *.bin *.a
	rm -f *.order *.markers regress
	rm -f *.gcno *.gcda *.out


# end of file
