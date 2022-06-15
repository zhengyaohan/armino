.PHONY: all clean help

ifeq ("$(MAKECMDGOALS)", "")
TARGET := default
else
TARGET := $(MAKECMDGOALS)
endif

all:
	echo "build target ${TARGET}"
	./build.sh ${TARGET}
clean:
	rm -rf build

help:
	echo "BK7236 FPGA secure flash layout"
	echo " ---------------------------  ---> 0x0200 0000"
	echo "| Control Partition (4K)    |
	echo " ---------------------------  ---> 0x0200 1000"
	echo "| Simu OTP Partition (4K)   |
	echo " ---------------------------  ---> 0x0200 2000"
	echo "| R Manifest Partition (4K) |
	echo " ---------------------------  ---> 0x0200 3000"
	echo "| R BL Partition (64K)      |
	echo " ---------------------------  ---> 0x0201 3000"
	echo "| P Manifest Partition (4K) |
	echo " ---------------------------  ---> 0x0201 4000"
	echo "| P BL Partition (64K)      |
	echo " ---------------------------  ---> 0x0202 4000"
	echo "| TFM-S Partition (384K)    |
	echo " ---------------------------  ---> 0x0208 4000"
	echo "| P Manifest Partition (4K) |
