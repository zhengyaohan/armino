PROJECT_DIR_DEFAULT := projects/legacy_app
PROJECT_BUILD_DIR_DEFAULT := build
ARMINO_TOOLCHAIN_DIR_DEFAULT := "/opt/gcc-arm-none-eabi-5_4-2016q3/bin"  
ARMINO_TOOLCHAIN_DIR_DEFAULT_4_RISCV := "/opt/risc-v/nds32le-elf-mculib-v5/bin"
ARMINO_SOC_DEFAULT := bk7231n
ARMINO_DIR := $(shell pwd)

ARMINO_TOOL := @$(ARMINO_DIR)/tools/build_tools/armino

# 1. soc_targets contains all supported SoCs
# 2. cmake_supported_targets contains all targets that can directly
#    passed to armino cmake build system
# 3. cmake_not_supported_targets contains all targets:
#    3.1> armino cmake doesn't support it, only implemented in this
#         Makefile
#    3.2> armino cmake supports it, but has different target name
soc_targets := $(shell find  middleware/arch/ -name "*.defconfig" -exec basename {} \; | cut -f1 -d ".")
cmake_supported_targets := menuconfig doc
cmake_not_supported_targets = help clean
all_targets = cmake_not_supported_targets soc_targets cmake_supported_targets
export SOC_SUPPORTED_TARGETS := ${soc_targets}
ARMINO_SOC := $(findstring $(MAKECMDGOALS), $(soc_targets))

ifeq ("$(ARMINO_SOC)", "")
	ARMINO_SOC := $(ARMINO_SOC_DEFAULT)
	ARMINO_TARGET := $(MAKECMDGOALS)
else
	ARMINO_TARGET := build
	PROJECT_BUILD_DIR_DEFAULT = build/$(ARMINO_SOC)
endif


ifdef PROJECT
	# Add script to validate the project dir
	PROJECT_DIR := projects/$(PROJECT)
else
	PROJECT_DIR := $(PROJECT_DIR_DEFAULT)
endif

ifdef BUILD_DIR
	PROJECT_BUILD_DIR := $(BUILD_DIR)
else
	PROJECT_BUILD_DIR := $(PROJECT_BUILD_DIR_DEFAULT)
endif

#TODO - remove it once we support install toolchain
ifdef TOOLCHAIN_DIR
	ARMINO_TOOLCHAIN_DIR := $(TOOLCHAIN_DIR)
else
	ARMINO_TOOLCHAIN_DIR := $(ARMINO_TOOLCHAIN_DIR_DEFAULT)
endif


.PHONY: all_targets

help:
	@echo ""
	@echo "usage: make {soc|menuconfig|help}"
	@echo ""
	@echo " make soc_type - rebuild specified soc"
	@echo "                 e.g. make bk7236"
	@echo " make build - fast build last soc"
	@echo " make menuconfig - confiure armino"
	@echo " make clean - clean build"
	@echo " make help - display this help info"
	@echo " make doc - generate doc"
	@echo ""

common:
	@echo "ARMINO_SOC is set to $(ARMINO_SOC)"
	@echo "ARMINO_TARGET is set to $(ARMINO_TARGET)"
	@echo "armino project path=$(PROJECT_DIR)"
	@echo "armino path=$(ARMINO_DIR)"
	@echo "armino toolchain path=$(ARMINO_TOOLCHAIN_DIR)"
	@echo "armino build path=$(ARMINO_DIR)/$(PROJECT_BUILD_DIR)"
	@export ARMINO_PATH=$(ARMINO_DIR)

has_lib_src := $(shell $(ARMINO_DIR)/tools/build_tools/detect_internal_lib_src.py)
ifeq ($(has_lib_src), 1)
libs: common
	@rm -rf components/bk_libs/$(ARMINO_SOC)
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) -T $(ARMINO_TOOLCHAIN_DIR) fullclean
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) -l set-target $(ARMINO_SOC)
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) -T $(ARMINO_TOOLCHAIN_DIR) -l internal_libs
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) -T $(ARMINO_TOOLCHAIN_DIR) fullclean

rm_libs:common
	@for soc in $(soc_targets); do echo "rmove components/bk_libs/$$soc"; rm -rf components/bk_libs/$$soc; done
	@echo rmove "$(PROJECT_DIR_DEFAULT)/sdkconfig"
	@rm -f $(PROJECT_DIR_DEFAULT)/sdkconfig
else
libs: common

rm_libs: common
	@echo rmove "$(PROJECT_DIR_DEFAULT)/sdkconfig"
	@rm -f $(PROJECT_DIR_DEFAULT)/sdkconfig
endif

$(soc_targets): libs
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) set-target $(ARMINO_SOC)
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) -T $(ARMINO_TOOLCHAIN_DIR) $(ARMINO_TARGET)

$(cmake_supported_targets): common
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) $(ARMINO_TARGET)

clean: rm_libs
	@$(ARMINO_TOOL) -B ./$(PROJECT_BUILD_DIR) -P ./$(PROJECT_DIR) fullclean
