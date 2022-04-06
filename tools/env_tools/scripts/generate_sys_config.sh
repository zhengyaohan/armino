#!/bin/bash
# generate_sys_config alias_project_name targe_sys_config
# example: 
# generate_sys_config bk7231u config/sys_config.h

ARM_GCC_TOOLCHAIN=${FREERTOS_EXEC_PATH}

if [ -z "${ECHO}" ]; then
ECHO=echo
fi

OS=`uname -s`
if [ "${OS}" = "Darwin" ]; then
MD5="md5 -r"
else
MD5=md5sum
fi

if [ "$2" == "" ]; then
	old_sys_config=config/sys_config.h
fi

if [ ! -d "config" ]; then
	mkdir config
fi

new_sys_config=beken378/components/bk_config/sys_config_$1.h

if [ -f $new_sys_config ]; then
	new_hash=`${MD5} $new_sys_config | cut -d' ' -f 1`
else
	echo "$new_sys_config not exist!"
	exit 1
fi

if [ -f $old_sys_config ]; then
	old_hash=`${MD5} $old_sys_config | cut -d' ' -f 1`
else
	old_hash=""
fi

#echo "hash($new_sys_config)=$new_hash"
#echo "hash($old_sys_config)=$old_hash"

if [ "$new_hash" != "$old_hash" ]; then
	cp -f $new_sys_config $old_sys_config
#	echo "update $old_sys_config with $new_sys_config"
fi

rm -f .platform
echo $1 > .platform

${ECHO} "  ${GREEN}GEN  .config${NC}"
rm -f .config
echo '#include "config/sys_config.h"' > config.c
sed -n '/^#define/p' config/sys_config.h | awk '{print $2}' | sort -d | uniq | awk '{print "valueOf_"$1"="$1}' >> config.c
echo "# Autogenerated by Makefile, DON'T EDIT" > .config
${ARM_GCC_TOOLCHAIN}arm-none-eabi-gcc -E config.c | grep '^valueOf_' | sed 's/valueOf_//' >> .config
sed -i '/_SYS_CONFIG_H_/d' .config
rm -f config.c
