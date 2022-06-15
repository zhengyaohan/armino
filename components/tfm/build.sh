ROOT_DIR=$(pwd)
TOOL_DIR=${ROOT_DIR}/tools
TFM_BIN_DIR=${ROOT_DIR}/tfm_build/bin
SEC_TOOL=${TOOL_DIR}/secure_boot_tool
IMG_TOOL=${TOOL_DIR}/image.py
export PATH=/opt/gcc-arm-none-eabi-10.3-2021.10/bin:$PATH

# build bootrom
deploy=$2
if [ "${deploy}" != "" ]; then
${ROOT_DIR}/deploy.sh ${ROOT_DIR} ${deploy}
fi

#rm -rf tfm_build
cd tfm
#cmake -S . -B ../tfm_build -DTFM_PLATFORM=arm/mps2/an521 -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake -DCMAKE_BUILD_TYPE=Debug -DTEST_S=ON -DTEST_NS=ON -DUSER=$USER
#cmake --build ../tfm_build -- install

config=$1
if [ "${config}" == "" ]; then
config=default
fi

BUILD_DIR=${ROOT_DIR}/build/${config}

mkdir -p ${ROOT_DIR}/build/
mkdir -p ${ROOT_DIR}/build/${config}/
mkdir -p ${ROOT_DIR}/build/${config}/config/
mkdir -p ${ROOT_DIR}/build/${config}/bin/

echo "ROOT_DIR=${ROOT_DIR}"
echo "TOOL_DIR=${TOOL_DIR}"
echo "BUILD_DIR=${BUILD_DIR}"
echo "TFM_BIN_DIR=${TFM_BIN_DIR}"

if [ -d ${ROOT_DIR}/config/${config}/bin ]; then
	TFM_BIN_DIR=${ROOT_DIR}/config/${config}/bin
	echo "Use small bin, location=${TFM_BIN_DIR}"
fi

cp -r ${ROOT_DIR}/config/${config}/* ${BUILD_DIR}/config
cp -r ${TFM_BIN_DIR}/*.bin ${BUILD_DIR}/bin

cd ${BUILD_DIR}/config
#${BUILD_DIR}/config/gen_key.sh

echo "Generate signature for bin"
cp -r ${TFM_BIN_DIR}/*.bin ${BUILD_DIR}/config
${SEC_TOOL} -k key_desc.json -m manifest_prim.json -o ${BUILD_DIR}/bin
mv ${BUILD_DIR}/config/*.pem ${BUILD_DIR}/bin/

echo "Skip Generate CRC bin"
#${IMG_TOOL} crc16 -i bl2.bin -o bl2_crc.bin
#${IMG_TOOL} crc16 -i tfm_ns.bin -o tfm_ns_crc.bin 
#${IMG_TOOL} crc16 -i tfm_s.bin -o tfm_s_crc.bin 

cd ${BUILD_DIR}/bin
echo "Generate all.bin"
${IMG_TOOL} merge -j ${BUILD_DIR}/config/img_config.json -o ${BUILD_DIR}/bin/all.bin
echo "Generate all.out"
${IMG_TOOL} asii -i ${BUILD_DIR}/bin/all.bin -o ${BUILD_DIR}/bin/all.out

echo "Generate OTP bin"
${IMG_TOOL} merge -j ${BUILD_DIR}/config/otp_config.json -o ${BUILD_DIR}/bin/otp.bin
echo "Generate OTP Revert bin"
${IMG_TOOL} revert -i ${BUILD_DIR}/bin/otp.bin -o ${BUILD_DIR}/bin/otp_revert.txt

echo "Remove temparory bin"

DST_DIR=/win30/ming.liu/for_pujie/bootrom_${deploy}
MODIFY_DIR=/win30/ming.liu/for_pujie/bootrom_${deploy}/otp_modify
if [ "${deploy}" != "" ]; then
	cp ${BUILD_DIR}/bin/* ${DST_DIR}/bin
	cp -r ${BUILD_DIR}/config ${DST_DIR}
	mkdir -p ${MODIFY_DIR}
	cp -r ${IMG_TOOL} ${MODIFY_DIR}/
	cp -r ${BUILD_DIR}/bin/puk_digest.bin ${MODIFY_DIR}/
	#cp -r ${BUILD_DIR}/config/otp_config.json ${MODIFY_DIR}/
	#cp -r ${BUILD_DIR}/config/README.md ${MODIFY_DIR}/
	#cp -r ${BUILD_DIR}/config/run.sh ${MODIFY_DIR}/
fi

echo ""
echo "BK7236 FPGA secure flash layout"
echo " ---------------------------   0x0200 0000"
echo "| Control Partition (4K)    |"
echo " ---------------------------   0x0200 1000"
echo "| Simu OTP Partition (4K)   |"
echo " ---------------------------   0x0200 2000"
echo "| R Manifest Partition (4K) |"
echo " ---------------------------   0x0200 3000"
echo "| P Manifest Partition (4K) |"
echo " ---------------------------   0x0200 4000"
echo "| R BL Partition (64K)      |"
echo " ---------------------------   0x0201 4000"
echo "| P BL Partition (64K)      |"
echo " ---------------------------   0x0202 4000"
echo "| TFM-S Partition (384K)    |"
echo " ---------------------------   0x0208 4000"
echo "| TFM-NS Partition (256K)   |"
echo " ---------------------------   0x020c 4000"
echo ""
echo "Find the config file in ${BUILD_DIR}/config"
echo "Find the build result in ${BUILD_DIR}/bin"


