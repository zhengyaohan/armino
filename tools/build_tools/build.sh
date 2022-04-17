#!/bin/bash

set -e
set -u

ARMINO_TARGET="build"
ARMINO_DIR=$1
PROJECT_DIR=$2
BUILD_DIR=$3
BUILD_TARGET=$4

ARMINO_TOOL=${ARMINO_DIR}/tools/build_tools/armino
BUILD_TARGET_PREFIX=${BUILD_TARGET:0:5}

need_build_properties_lib=0
need_build_soc=0
need_clean=0

if [ "${BUILD_TARGET_PREFIX}" == "libbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#lib}
	need_build_properties_lib=1
	BUILD_DIR=build/properties_libs/${ARMINO_SOC}
	PROJECT_DIR=projects/properties_libs
	echo "build armino ${ARMINO_SOC} properties libs only"
elif [ "${BUILD_TARGET_PREFIX}" == "relbk" ]; then
	ARMINO_SOC=${BUILD_TARGET#rel}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	need_build_properties_lib=0
	need_build_soc=1
	echo "build armino ${ARMINO_SOC} only"
elif [ "${BUILD_TARGET_PREFIX}" == "clean" ]; then
	ARMINO_SOC=${BUILD_TARGET#clean}
	BUILD_DIR=build/${BUILD_DIR}/${ARMINO_SOC}
	need_clean=1
else
	ARMINO_SOC=${BUILD_TARGET}
	need_build_soc=1
	has_properties_lib_src=$(${ARMINO_DIR}/tools/build_tools/detect_internal_lib_src.py)
	if [ ${has_properties_lib_src} == "1" ]; then
		echo "build armino properties lib first, then ${ARMINO_SOC}"
		need_build_properties_lib=1
	else
		need_build_properties_lib=0
	fi
fi

PROPERTIES_LIB_BUILD_DIR=${ARMINO_DIR}/build/properties_libs/${ARMINO_SOC}
PROPERTIES_LIB_DIR=${ARMINO_DIR}/projects/properties_libs

if [ "${need_clean}" == "1" ]; then
	echo "remove ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}"
	rm -rf ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}
	echo "remove ${ARMINO_DIR}/${BUILD_DIR}"
	rm -rf ${ARMINO_DIR}/${BUILD_DIR}
	echo "remove ${PROPERTIES_LIB_BUILD_DIR}"
	rm -rf ${PROPERTIES_LIB_BUILD_DIR}
	exit 0
fi

if [ "${need_build_properties_lib}" == "1" ]; then
	echo "build properties lib for ${ARMINO_SOC}"
	rm -rf ${PROPERTIES_LIB_BUILD_DIR}/sdkconfig
	rm -rf ${ARMINO_DIR}/components/bk_libs/${ARMINO_SOC}
	${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} set-target ${ARMINO_SOC}
	${ARMINO_TOOL} -B ${PROPERTIES_LIB_BUILD_DIR} -P ${PROPERTIES_LIB_DIR} ${ARMINO_TARGET}
	${ARMINO_DIR}/tools/build_tools/copy_internal_libs.sh ${ARMINO_SOC} ${ARMINO_DIR} ${PROPERTIES_LIB_BUILD_DIR}
fi

if [ "${need_build_soc}" == "1" ]; then
	echo "build ${ARMINO_SOC}"
	rm -rf ${ARMINO_DIR}/${BUILD_DIR}/sdkconfig
	${ARMINO_TOOL} -B ${ARMINO_DIR}/${BUILD_DIR} -P ${ARMINO_DIR}/${PROJECT_DIR} set-target ${ARMINO_SOC}
	${ARMINO_TOOL} -B ${ARMINO_DIR}/${BUILD_DIR} -P ${ARMINO_DIR}/${PROJECT_DIR} ${ARMINO_TARGET}
fi

