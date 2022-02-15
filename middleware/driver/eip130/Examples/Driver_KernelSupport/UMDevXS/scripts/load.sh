#!/bin/sh
#
# load.sh
#
# This script loads the Kernel Support Driver (UMDevXS) and creates
# the communication point for the application (using the proxy).
#

#############################################################################
# Copyright (c) 2014-2018 INSIDE Secure B.V. All Rights Reserved.
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


DIR_NAME_DRIVER=UMDevXS

if [ $HOST_HW_PLATFORM = "FPGA_Zynq_ZC702" ]
then
    KO_NAME_DRIVER=umdevxs_k.ko
    CHAR_NAME_DEVICE=umdevxs_c
elif [ $HOST_HW_PLATFORM = "FPGA_V2M_Juno" ]
then
    KO_NAME_DRIVER=umdevxs_k.ko
    CHAR_NAME_DEVICE=umdevxs_c
elif [ $HOST_HW_PLATFORM = "FPGA_Virtex6_PCI" ]
then
    KO_NAME_DRIVER=umpci_k.ko
    CHAR_NAME_DEVICE=umpci_c
else
    echo "Unsupported HOST_HW_PLATFORM $HOST_HW_PLATFORM"
    exit 1
fi

dmesg -c > /dev/null 2>&1
if [ $? -ne 0 ]
then
    echo "Not enough rights. Try sudo."
    exit
fi

# check if already loaded (0) or not yet loaded (1)
is_loaded=$(lsmod | grep $CHAR_NAME_DEVICE > /dev/null; echo $?)
if [ $is_loaded -eq 0 ]
then
    echo "Already loaded; use unload.sh first"
    exit
fi

# check if the driver binary location is provided, if not use default
if [ $# -eq 1 ]
then
    build=$1/$KO_NAME_DRIVER
else
    build=../../Driver_KernelSupport/$DIR_NAME_DRIVER/build/$KO_NAME_DRIVER
fi

if [ ! -f "$build" ]
then
    echo "Fatal: cannot find driver"
    exit
fi

# load the driver
insmod $build

# grab the major number for the character device
major_nr=`awk "\\$2==\"$CHAR_NAME_DEVICE\" {print \\$1}" /proc/devices`
#echo "Major nr: $major_nr"

# make the communication pointer
mknod /dev/$CHAR_NAME_DEVICE c ${major_nr} 0

# make accessible to application
chmod 666 /dev/$CHAR_NAME_DEVICE

# print and clear start-up messages
dmesg -c

# end of file load.sh
