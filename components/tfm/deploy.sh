ROOT_DIR=${1}
BOOTROM_DIR=${ROOT_DIR}/bk7236_bootrom_s/BootROM

dst=$2
if [ "${dst}" == "" ]; then
echo "Should privde destination dir"
exit 1
fi

DST_DIR=/win30/ming.liu/for_pujie/bootrom_${dst}

if [ ! -d ${BOOTROM_DIR} ]; then
git clone git@192.168.0.6:wangzhilei/bk7236_bootrom_s.git
fi

cd ${BOOTROM_DIR}
make clean
make
cd -

mkdir -p ${DST_DIR}
rm -rf ${DST_DIR}/*
mkdir -p ${DST_DIR}/bin

cp ${BOOTROM_DIR}/../out/release/bk7236_v2022/ubl/bootrom/bootrom.bin ${DST_DIR}/bin
cp ${BOOTROM_DIR}/../out/release/bk7236_v2022/ubl/bootrom/bootrom.elf ${DST_DIR}/bin

