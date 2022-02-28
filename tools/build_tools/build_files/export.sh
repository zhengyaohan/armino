#!/usr/bin/env bash

#set -e
#set -u

# Configure SDK path
export ARMINO_PATH=$(pwd)

# Configure Project path
ARMINO_DEFAULT_PROJECT_PATH="${ARMINO_PATH}/projects/legacy_app"
if [ "${ARMINO_PROJECT_PATH}" == "" ]; then
	echo "armino project path is not set, use default project path"
	echo "armino default project path=${ARMINO_DEFAULT_PROJECT_PATH}"
	export ARMINO_PROJECT_PATH=${ARMINO_DEFAULT_PROJECT_PATH}
else
	echo "armino project path=${ARMINO_PROJECT_PATH}"
fi

# Configure toolchain path
ARMINO_DEFAULT_TOOLCHAIN_PATH="/opt/gcc-arm-none-eabi-5_4-2016q3/bin"
if [ "${ARMINO_TOOLCHAIN_PATH}" == "" ]; then
	echo "armino toolchain path is not set, use default toolchain path"
	echo "armino default toolchain path=${ARMINO_DEFAULT_TOOLCHAIN_PATH}"
	export ARMINO_TOOLCHAIN_PATH=${ARMINO_DEFAULT_TOOLCHAIN_PATH}
else
	echo "armino toolchain path=${ARMINO_TOOLCHAIN_PATH}"
fi

# Configure armino toolchain path for using xxx-objdump and other build tools
armino_toolchain_path_included=$(echo ${PATH} | grep "${ARMINO_TOOLCHAIN_PATH}")
if [ "${armino_toolchain_path_included}" == "" ]; then
	export PATH="${PATH}:${ARMINO_TOOLCHAIN_PATH}"
fi

# Configure armino tool path to PATH for using 'armino'
armino_tool_path_include=$(echo "${PATH}" | grep "${ARMINO_PATH}/tools/build_tools")
if [ "${armino_tool_path_include}" == "" ]; then
	export PATH="${ARMINO_PATH}/tools/build_tools:${PATH}"
fi
