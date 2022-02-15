#!/bin/sh
#
# This file unloads the test components loaded by load_vip.sh
#
# This script does not capture or reset dmesg output.
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

rm -rf /dev/vexp_c
rmmod driver_vip_k


# end of file
