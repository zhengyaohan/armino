#!/bin/sh
#
# unload.sh
#
# This script unloads the Kernel Support Driver and deletes the communication
# point for the application (using the proxy).
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

# print and clear messages
dmesg -c 2>/dev/null

# check that we run with root rights
# (this assumes dmesg -c is root-only)
if [ $? -ne 0 ]
then
    echo "Not enough rights. Try sudo."
    exit
fi

# remove the communication point
rm -f /dev/$CHAR_NAME_DEVICE

# unload the driver
rmmod $KO_NAME_DRIVER

# print and clear the shutdown-messages
dmesg -c

# end of script unload.sh
