#
# Copyright (c) 2012-2021 Andes Technology Corporation
# All rights reserved.
#

make_dir := $(shell pwd)
make_clr_l :=  $(shell pwd)/l_boot
make_clr_up := $(shell pwd)/up_boot

subsystem:
	cd $(make_dir)/l_boot && $(MAKE)
	cd $(make_dir)/up_boot && $(MAKE)

clean:
	@rm -f $(make_dir)/*.elf
	@rm -f $(make_dir)/*.bin
	@rm -f $(make_dir)/*.asm
	@rm -f $(make_dir)/*.map
	cd $(make_dir)/l_boot && make clean
	cd $(make_dir)/up_boot && make clean

	
