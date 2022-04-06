#!/bin/bash
#
# This script runs the Secure Debug demo application once.
# 1. Load Driver kernel module
# 2. Load da_securedebug kernel module
# 3. Capture results
# 4. Unload da_securedebug
# 5. Unload Driver
#
# Unless a path is provided, this script will load the components from their
# default location in the DDK, relative to the location of the script itself.
# The optional directory pointed to must contain all binaries.
#
# This script makes dmesg output and clear its message buffer.
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
      echo "[run_da_securedebug.sh] Missing file $1"
      exit 1
  fi
}

# main script starts here

KO_NAME_DRIVER=driver_val_k.ko
KO_NAME_DEMOAPP=da_securedebug_k.ko

# move to the script base directory
MY_DIR=$(dirname $0)
cd $MY_DIR

if [ $# -eq 0 ]
then
    PATH_DRIVER=../../Driver/build
    PATH_DEMOAPP=../build
else
    PATH_DRIVER=$1
    PATH_DEMOAPP=$1
fi

# make sure the components are in place
check_file_exists $PATH_DRIVER/$KO_NAME_DRIVER
check_file_exists $PATH_DEMOAPP/$KO_NAME_DEMOAPP

# empty dmesg output prior to starting DEMOAPP
dmesg -c > /dev/null

# load the kernel modules
insmod $PATH_DRIVER/$KO_NAME_DRIVER

# capture load messages
dmesg -c

# following line starts execution of the tests
insmod $PATH_DEMOAPP/$KO_NAME_DEMOAPP

# capture test execution results
dmesg -c

rmmod $KO_NAME_DEMOAPP
rmmod $KO_NAME_DRIVER

# end of file
