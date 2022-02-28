#!/bin/sh
#
# This script loads the components required to run the VAL Test Tool.
# 1. Load driver kernel module
# 2. Make node /dev/vexp_c (used by user-space apps)
#
# Unless a path is provided, this script will load the components from their
# default location in the DDK, relative to the location of the script itself.
# The optional directory pointed to must contain all binaries.
#
# This file does not reset or capture dmesg output.
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

#-------------------------------------------------------------------------------
# check_file_exists
#
# Arguments:
#  1. path to file to check, may include whitespaces.
#
# This function checks if the given file exists. The function returns when
# the file exists. Otherwise a message is printed and the script is aborted
# with exit code 1.
#
check_file_exists()
{
  if [ ! -f "$1" ]
  then
      echo "[load_val.sh] Missing file $1"
      exit 1
  fi
}

# main script starts here
KO_NAME_DRIVER=driver_val_k.ko

if [ $# -eq 0 ]
then
    PATH_DRIVER=../../Driver_VAL/build
else
    PATH_DRIVER=$1
fi

# make sure the components are in place
check_file_exists $PATH_DRIVER/$KO_NAME_DRIVER

# clean up
rm -rf /dev/vexp_c

# load the kernel modules
insmod $PATH_DRIVER/$KO_NAME_DRIVER

major_number=`awk "\\$2==\"vexp_c\" {print \\$1}" /proc/devices`
mknod /dev/vexp_c c ${major_number} 0


# end of file
