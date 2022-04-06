#!/bin/sh
#
# This script executes the regression test for all pre-defined configurations.
#
# This script depends on the Regress_vip/regress_vip.sh script.
#
# Command line arguments: none

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

# move to the script base directory
MY_DIR=$(dirname $0)
cd $MY_DIR

BUILDS_DIR=$(pwd)/Builds

VALGRIND=$VALGRIND_BIN

COVER_OUT=

# reset coverage counters
if [ "$COVER_DDK" = 'y' ]
then
    lcov -z > /dev/null 2>&1

    if [ -d "$COVER_OUTPUT_DIR" ]
    then
        rm -rf $COVER_OUTPUT_DIR/Coverage
    else
        rm -rf $(pwd)/Coverage
    fi
fi

mkdir -p Logs

# regress all driver configurations
if [ "$BUILD_CONFIGS" = "" ]
then
    # No support for interrupt-enabled configurations yet
    #BUILD_CONFIGS="C0 C1 C2 C3 C4 C5"
    BUILD_CONFIGS="C0 C2 C3"
fi

NoResetAllowed=""
if [ "$1" = "-r" ]
then
    NoResetAllowed="-r"
fi

for ConfigLp in $BUILD_CONFIGS
do
    echo "[RegressAll] Starting regression of $ConfigLp"
    ./regress_vip.sh $ConfigLp $NoResetAllowed
done

echo "[RegressAll] Completed all configurations"

# end of file
