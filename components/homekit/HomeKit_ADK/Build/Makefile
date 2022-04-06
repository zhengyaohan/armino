.PHONY: apps tests tools clean static shared install

.SECONDARY:

all: tests apps

PLATFORM := $(shell uname)

PAL ?= $(PLATFORM)
BUILD_TYPE ?= Debug
USE_CCACHE ?= 1
CODESIGN ?= 0
CFLAGS := -Wall -Werror -MMD -MP -Wpointer-arith -Wsign-compare
CFLAGS_Debug :=
CFLAGS_Test :=
CFLAGS_Release :=
CFLAGS_FEATURES_IP :=
CFLAGS_FEATURES_BLE :=
CFLAGS_FEATURES_THREAD :=
CFLAGS_FEATURES_APP :=
LDFLAGS :=

BUILD_TYPES := Debug Test Release Tool

# Include build profiles makefile
-include Build/Makefile.Profiles

# Include build sensors makefile
-include Build/Makefile.Sensors

# Include feature definition makefile
-include Build/Makefile.Features

# Include the platform specific makefile
-include Build/Makefile.$(PAL)

# ADK Version information
-include Build/Makefile.Version
CFLAGS += -DHAP_VERSION="$(VERSION_MAJOR).$(VERSION_MINOR)"
CFLAGS += -DHAP_COMPILER_VERSION="$(COMPILER_VERSION)"
ifdef BUILD_ID
CFLAGS += -DHAP_BUILD="$(BUILD_ID)"
endif

CFLAGS += -ICommon/include

# Set logging level for each build type
ifeq ($(LOG_LEVEL),)
	LOG_LEVEL_Debug := 4    # FAULT, ERROR, DEFAULT, INFO, DEBUG
	LOG_LEVEL_Test := 1     # FAULT, ERROR
	LOG_LEVEL_Release := 1  # FAULT, ERROR
	LOG_LEVEL_Tool := 1     # FAULT, ERROR
else
	LOG_LEVEL_Debug := $(LOG_LEVEL)
	LOG_LEVEL_Test := $(LOG_LEVEL)
	LOG_LEVEL_Release := $(LOG_LEVEL)
	LOG_LEVEL_Tool := $(LOG_LEVEL)
endif

# Override maximum length of a log message if provided
ifneq ($(HAP_LOG_MESSAGE_MAX_BYTES),)
CFLAGS_Debug += -DHAP_LOG_MESSAGE_MAX_BYTES=$(HAP_LOG_MESSAGE_MAX_BYTES)
CFLAGS_Test += -DHAP_LOG_MESSAGE_MAX_BYTES=$(HAP_LOG_MESSAGE_MAX_BYTES)
CFLAGS_Release += -DHAP_LOG_MESSAGE_MAX_BYTES=$(HAP_LOG_MESSAGE_MAX_BYTES)
endif

# Override maximum number of stored metric events if provided
ifneq ($(HAP_METRICS_MAX_STORED_EVENTS),)
CFLAGS_Debug += -DHAP_METRICS_MAX_STORED_EVENTS=$(HAP_METRICS_MAX_STORED_EVENTS)
CFLAGS_Test += -DHAP_METRICS_MAX_STORED_EVENTS=$(HAP_METRICS_MAX_STORED_EVENTS)
CFLAGS_Release += -DHAP_METRICS_MAX_STORED_EVENTS=$(HAP_METRICS_MAX_STORED_EVENTS)
endif

# Override maximum length of HDS Rx buffer log if provided
ifneq ($(HAP_HDS_RX_BUFFER_LOG_MAX_BYTES),)
CFLAGS_Debug += -DHAP_HDS_RX_BUFFER_LOG_MAX_BYTES=$(HAP_HDS_RX_BUFFER_LOG_MAX_BYTES)
CFLAGS_Test += -DHAP_HDS_RX_BUFFER_LOG_MAX_BYTES=$(HAP_HDS_RX_BUFFER_LOG_MAX_BYTES)
CFLAGS_Release += -DHAP_HDS_RX_BUFFER_LOG_MAX_BYTES=$(HAP_HDS_RX_BUFFER_LOG_MAX_BYTES)
endif

ifeq ($(BUILD_TYPE),Release)
ENABLE_HAP_TESTING ?= 0
else
ENABLE_HAP_TESTING ?= 1
endif

ENABLE_BUFFER_LOGS ?= 1
CFLAGS_Debug += \
	-DHAP_LOG_LEVEL=$(LOG_LEVEL_Debug) \
	-DHAP_ENABLE_BUFFER_LOGS=$(ENABLE_BUFFER_LOGS) \
	-DHAP_TESTING=$(ENABLE_HAP_TESTING) \
	-DHAP_LOG_SENSITIVE

CFLAGS_Test +=  \
	-DHAP_LOG_LEVEL=$(LOG_LEVEL_Test) \
	-DHAP_ENABLE_BUFFER_LOGS=$(ENABLE_BUFFER_LOGS) \

CFLAGS_Release += \
	-DHAP_DISABLE_ASSERTS=1 \
	-DHAP_DISABLE_PRECONDITIONS=1 \
	-DHAP_LOG_LEVEL=$(LOG_LEVEL_Release) \
	-DHAP_TESTING=$(ENABLE_HAP_TESTING)

CFLAGS_Tool += \
	-DHAP_LOG_LEVEL=$(LOG_LEVEL_Tool) \
	-DHAP_FEATURE_CAMERA=0 \

# Address sanitizer
ifeq ("$(notdir $(filter-out ccache,$(CC)))", "clang")
ASAN ?= 1
endif
ifeq ($(ASAN), 1)
CFLAGS_Debug += -fsanitize=address
CFLAGS_Test += -fsanitize=address
endif

ifneq ($(HOMEKITSTORE_DIR),)
    CFLAGS += -DHOMEKITSTORE_DIR='"$(HOMEKITSTORE_DIR)"'
endif

# Protocol specifc flags only used by Applications
CFLAGS_IP := -DHAVE_IP=1
CFLAGS_BLE := -DHAVE_BLE=1
CFLAGS_THREAD := -DHAVE_THREAD=1

# Setup HAP modules
SRC_DIRS_HAP += HAP External/HTTP External/JSON External/Base64
SRC_DIRS_HAP += HAP/Thread
SRC_DIRS_HAP += HAP/MetricEvents

SRC_DIRS := $(SRC_DIRS_HAP) PAL $(SRC_DIRS_$(PAL))

CFLAGS += $(CFLAGS_$(PAL))
C_ONLY_FLAGS += $(C_ONLY_FLAGS_$(PAL))
LDFLAGS += $(LDFLAGS_$(PAL))
C_ONLY_LDFLAGS += $(C_ONLY_LDFLAGS_$(PAL))
CRYPTO_DIRS += $(CRYPTO_$(PAL))

OUTPUT_DIR := Output/$(PAL)-$(COMPILER_TARGET)

# Compile against the selected PAL except unit tests, which always use the Mock PAL
CFLAGS_Debug += $(addprefix -I,PAL/$(PAL) $(SRC_DIRS_$(PAL)))
CFLAGS_Test += $(addprefix -I,PAL/Mock)
CFLAGS_Test += $(CFLAGS_Test_$(PAL))
CFLAGS_Tool += $(addprefix -I,PAL/$(PAL) $(SRC_DIRS_$(PAL)))

CFLAGS_Release += $(addprefix -I,PAL/$(PAL) $(SRC_DIRS_$(PAL)))

SRC_DIRS_PAL := $(foreach src,$(SRC_DIRS_$(PAL)),$(src))

SRC_DIRS_MOCK := PAL PAL/Mock
SRC_DIRS_MOCK += $(SRC_DIRS_MOCK_$(PAL))
SRC_DIRS_MOCK += PAL/Thread/Mock

CRYPTO_MODULES := $(subst PAL/Crypto/,,$(CRYPTO_DIRS))

# Pick the first crypto library supported by the platform if no makefile option is provided
CRYPTO ?= $(firstword $(CRYPTO_MODULES))

# We support building C and Objective C code
SRC_EXTS := c m S

# Find all the source files in the given directories
all_sources_in = $(wildcard $(foreach ext,$(SRC_EXTS),$(addsuffix /*.$(ext), $(1))))

# C++ source extensions
CXXSRC_EXTS := cpp cxx c++ cc

# Find all C++ source files in the given directories
all_cxxsources_in = $(wildcard $(foreach ext,$(CXXSRC_EXTS),$(addsuffix /*.$(ext), $(1))))

# Make each source directory an include path (except PAL implementations)
CFLAGS += $(addprefix -I,$(filter-out PAL/%,$(SRC_DIRS)))

# Include common application files
APPS_COMMON := Applications/Common/DB Applications/Common/Helper Applications/Common/Platform Applications/Common/Platform/$(PAL)
CFLAGS += $(addprefix -I, $(APPS_COMMON))

# Add crypto module implementations include paths
CFLAGS += $(CFLAGS_$(CRYPTO))

# Concatenate lists
noop =
space = $(noop) $(noop)
combine = $(subst $(space),$(2),$(1))

# Turn a source list into object file names
# 1 - $(build_type)
# 2 - $(protocol)/$(app)
# 3 - suffix (.o, .a, .OpenSSL)
to_output = $(addprefix $(OUTPUT_DIR)/$(1)/,$(addsuffix $(3),$(basename $(2))))

# 1 - $(build_type)
# 2 - $(protocol)/$(app)
to_object = $(call to_output,$(1),$(2),.o)

# 1 - $(build_type)
# 2 - $(protocol)/$(app)
to_archive = $(call to_output,$(1),$(2),.a)

#to_executable: create a label for an executable in the format $(OUT)/$(PROTOCOL)/$(APP).$(CRYPTO)
# 1 - $(build_type)
# 2 - $(protocol)/$(app)
# 3 - $(crypto)
to_executable = $(call to_output,$(1),$(2),.$(call combine,$(3),.))

#to_library: create a label for an shared library in the format $(OUT)/$(PROTOCOL)/libADK.$(APP).$(CRYPTO).$(SO_EXT)
# 1 - $(build_type)
# 2 - $(protocol)/$(app)
# 3 - $(crypto)
# 4 - $(SO_EXT)
to_library = $(call to_output,$(1),$(word 1,$(subst /, ,$(2)))/$(word 2,$(subst /, ,$(2)))/,libADK.$(word 3,$(subst /, ,$(2))).$(call combine,$(3),.).$(4))

DEPS := $(shell find $(OUTPUT_DIR) -name *.d 2>/dev/null)

# Compile ($1=build type, $2=sources, $3=deps, $4=extra CFLAGS, $5= Application specific CFLAGS)
define compile
$(OUTPUT_DIR)/$(1)/$(2): $(3)
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) $(C_ONLY_FLAGS) $(CFLAGS_$(1)) $(C_ONLY_FLAGS_$(1)) $($(subst .,CFLAGS_,$(suffix $3))) $(4) $(5) -DHAP_$(1) -c $$< -o $$@

endef

# C++ Compile ($1=build type, $2=sources, $3=deps, $4=extra CFLAGS)
define cxxcompile
$(OUTPUT_DIR)/$(1)/$(2): $(3)
	@mkdir -p $$(dir $$@)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(CFLAGS_$(1)) $(CXXFLAGS_$(1)) $($(subst .,CFLAGS_,$(suffix $3))) $(4) -DHAP_$(1) -c $$< -o $$@

endef

$(eval $(foreach build_type,$(BUILD_TYPES),$(foreach ext,$(SRC_EXTS),$(call compile,$(build_type),%.o,%.$(ext),))))

# Build modules ($1=build type, $2=target name, $3=sources)
define build_module_template
$(call to_archive,$(1),$(2)): $(call to_object,$(1),$(3))
	@mkdir -p $$(dir $$@)
	$$(AR) cr $$@ $$^

endef

# build_module($1=target name, $2=sources)
build_module = $(eval $(foreach build_type,$(BUILD_TYPES),$(call build_module_template,$(build_type),$(1),$(2))))

$(call build_module,sdk,$(SDK_SRCS_$(PAL)))
$(call build_module,hap,$(filter-out $(EXCLUDE_$(PAL)),$(call all_sources_in,$(SRC_DIRS_HAP))))
$(call build_module,$(PAL),$(filter-out $(EXCLUDE_$(PAL)),$(call all_sources_in,PAL $(SRC_DIRS_PAL)) $(call all_cxxsources_in,PAL $(SRC_DIRS_PAL))))
$(call build_module,Mock,$(filter-out $(EXCLUDE_$(PAL)),$(call all_sources_in,$(SRC_DIRS_MOCK))))

$(foreach crypto,$(CRYPTO_MODULES),$(call build_module,$(crypto),$(call all_sources_in,PAL/Crypto/$(crypto))))

CORE = hap $(SDK_$(PAL))

# Link executables
# 1 - $(build_type)
# 2 - $(protocol)/$(app)
# 3 - $(crypto)
# 4 - $(exe_src$(PAL))
# 5 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
define link_template
$(call to_executable,$(1),$(2),$(3)): $(call to_object,$(1),$(4)) $(call to_archive,$(1),$(5))
	mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) $(C_ONLY_FLAGS) $(CFLAGS_$(1)) $($(addprefix CFLAGS_,$(3))) -o $$@ $(LINK_BEGIN_$(PAL)) $(LDFLAGS) $(C_ONLY_LDFLAGS) $(LDFLAGS_$(1)) $(C_ONLY_LDFLAGS_$(1)) $($(addprefix LDFLAGS_,$(3))) $$^ $(LINK_END_$(PAL))
ifdef EXTRA_LINK_STEP_$(PAL)
	$(call EXTRA_LINK_STEP_$(PAL),$$@)
endif

ifeq ($(CODESIGN),1)
	codesign -s - $$@
endif

endef

# Link executables
# 1 - $(build_type)
# 2 - $(protocol)/$(app)
# 3 - $(crypto)
# 4 - $(exe_src$(PAL))
# 5 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
define cxxlink_template
$(call to_executable,$(1),$(2),$(3)): $(call to_object,$(1),$(4)) $(call to_archive,$(1),$(5))
	mkdir -p $$(dir $$@)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(CFLAGS_$(1)) $($(addprefix CFLAGS_,$(3))) -o $$@ $(LINK_BEGIN_$(PAL)) $(LDFLAGS) $(LDXXFLAGS) $(LDFLAGS_$(1)) $(LDXXFLAGS_$(1)) $($(addprefix LDFLAGS_,$(3))) $$^ $(LINK_END_$(PAL))
ifdef EXTRA_LINK_STEP_$(PAL)
	$(call EXTRA_LINK_STEP_$(PAL),$$@)
endif

ifeq ($(CODESIGN),1)
	codesign -s - $$@
endif

endef

# 1 - $(protocol)/$(app)
# 2 - $(crypto)
# 3 - $(exe_src$(PAL))
# 4 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
build_executable = $(eval $(foreach build_type,$(BUILD_TYPES),$(call link_template,$(build_type),$(1),$(2),$(3),$(4))))

# 1 - $(protocol)/$(app)
# 2 - $(crypto)
# 3 - $(exe_src$(PAL))
# 4 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
cxxbuild_executable = $(eval $(foreach build_type,$(BUILD_TYPES),$(call cxxlink_template,$(build_type),$(1),$(2),$(3),$(4))))

# Generate dynamic lib
# 1 - $(build_type)
# 2 - $(protocol)/$(app)
# 3 - $(crypto)
# 4 - $(exe_src$(PAL))
# 5 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
# 6 - $(so_ext)
define link_dyntemplate
$(call to_library,$(1),$(2),$(3),$(6)): $(call to_object,$(1),$(4)) $(call to_archive,$(1),$(5))
	mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) $(C_ONLY_FLAGS) $(CFLAGS_$(1)) $($(addprefix CFLAGS_,$(3))) -shared -o $$@ $(LINK_BEGIN_$(PAL)) $(LDFLAGS) $(C_ONLY_LDFLAGS) $(LDFLAGS_$(1)) $(C_ONLY_LDFLAGS_$(1)) $($(addprefix LDFLAGS_,$(3))) $$^ $(LINK_END_$(PAL))
ifeq ($(CODESIGN),1)
	codesign -s - $$@
endif

endef

# Generate dynamic lib
# 1 - $(build_type)
# 2 - $(protocol)/$(app)
# 3 - $(crypto)
# 4 - $(exe_src$(PAL))
# 5 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
# 6 - $(so_ext)
define cxxlink_dyntemplate
$(call to_library,$(1),$(2),$(3),$(6)): $(call to_object,$(1),$(4)) $(call to_archive,$(1),$(5))
	mkdir -p $$(dir $$@)
	$(CXX) $(CFLAGS) $(CXX_FLAGS) $(CFLAGS_$(1)) $($(addprefix CFLAGS_,$(3))) -shared -o $$@ $(LINK_BEGIN_$(PAL)) $(LDFLAGS) $(LDXXFLAGS) $(LDFLAGS_$(1)) $(LDXXFLAGS_$(1)) $($(addprefix LDFLAGS_,$(3))) $($(addprefix LDXXFLAGS_,$(3))) $$^ $(LINK_END_$(PAL))
ifeq ($(CODESIGN),1)
	codesign -s - $$@
endif

endef

# 1 - $(protocol)/$(app)
# 2 - $(crypto)
# 3 - $(exe_src$(PAL))
# 4 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
# 5 - $(so_ext)
build_library = $(eval $(foreach build_type,$(BUILD_TYPES),$(call link_dyntemplate,$(build_type),$(1),$(2),$(3),$(4),$(5))))

# 1 - $(protocol)/$(app)
# 2 - $(crypto)
# 3 - $(exe_src$(PAL))
# 4 - $(protocol)/$(app) $(CORE) $(PAL) $(crypto)
# 5 - $(so_ext)
cxxbuild_library = $(eval $(foreach build_type,$(BUILD_TYPES),$(call cxxlink_dyntemplate,$(build_type),$(1),$(2),$(3),$(4),$(5))))

# Protocols supported on the platform
PROTOCOLS ?= $(PROTOCOLS_$(PAL))

ifeq (,$(filter IP BLE THREAD,$(PROTOCOLS)))
    $(error unknown protocol in PROTOCOLS list. BLE IP THREAD supported, got $(PROTOCOLS))
endif

# Build apps
ifeq ($(APPS),)
	APPS := $(sort $(shell find Applications -maxdepth 1 -type d))
	APPS_LIST := $(filter-out Applications/Common Applications $(EXCLUDE_$(PAL)),$(APPS))
else
	APPS_LIST := $(addprefix Applications/,$(APPS))
endif

# Exclude Apps that are not supported on BLE
EXCLUDE_BLE := \
	Applications/IPCameraBridge \
	Applications/IPCameraEventRecorder \
	Applications/IPCameraRecorderBridge \
	Applications/IPCamera \
	Applications/WiFiRouter

$(foreach protocol,$(PROTOCOLS),$(foreach build_type,$(BUILD_TYPES),$(foreach ext,$(SRC_EXTS),$(eval $(call compile,$(build_type),$(protocol)/%.o,%.$(ext),$(CFLAGS_$(protocol)),$(CFLAGS_FEATURES_APP))))))
$(foreach protocol,$(PROTOCOLS),$(foreach app,$(filter-out $(EXCLUDE_$(protocol)),$(APPS_LIST)),$(call build_module,$(protocol)/$(app),$(addprefix $(protocol)/,$(filter-out $(EXCLUDE_$(PAL)),$(call all_sources_in,$(app))) $(foreach app_common,$(APPS_COMMON), $(filter-out $(EXCLUDE_$(PAL)),$(call all_sources_in,$(app_common))))))))
$(foreach protocol,$(PROTOCOLS),$(foreach app,$(filter-out $(EXCLUDE_$(protocol)),$(APPS_LIST)),$(foreach crypto,$(CRYPTO_MODULES),$(call build_executable,$(protocol)/$(app),$(crypto),$(EXE_SRCS_$(PAL)) $(protocol)/Applications/Main.c,$(protocol)/$(app) $(CORE) $(PAL) $(crypto) $(APP_LIB_$(PAL)) $($(CRYPTO)_LIB_$(PAL))))))
$(foreach protocol,$(PROTOCOLS),$(foreach app,$(filter-out $(EXCLUDE_$(protocol)),$(APPS_LIST)),$(foreach crypto,$(CRYPTO_MODULES),$(call build_library,$(protocol)/$(app),$(crypto),$(EXE_SRCS_$(PAL)) $(protocol)/Applications/Main.c,$(protocol)/$(app) $(CORE) $(PAL) $(crypto) $(APP_LIB_$(PAL)) $($(CRYPTO)_LIB_$(PAL)),$(SO_EXT)))))

info:
	@echo "Compiler: $(COMPILER_VERSION)"
	@echo "PAL: $(PAL)"
	@echo "Crypto modules: $(CRYPTO_MODULES) (default: $(CRYPTO))"
	@echo "APPS = $(APPS)"
	@echo "APP_LIST = $(APPS_LIST)"

apps: $(foreach protocol,$(PROTOCOLS),$(foreach app,$(filter-out $(EXCLUDE_$(protocol)),$(APPS_LIST)),$(call to_executable,$(BUILD_TYPE),$(protocol)/$(app),$(CRYPTO))))

static: $(foreach protocol,$(PROTOCOLS),$(foreach app,$(filter-out $(EXCLUDE_$(protocol)),$(APPS_LIST)),$(call to_archive,$(BUILD_TYPE),$(protocol)/$(app) $(CORE) $(PAL) $(CRYPTO))))

shared: $(foreach protocol,$(PROTOCOLS),$(foreach app,$(filter-out $(EXCLUDE_$(protocol)),$(APPS_LIST)),$(call to_library,$(BUILD_TYPE),$(protocol)/$(app),$(CRYPTO),$(SO_EXT))))

clean:
	rm -rf $(OUTPUT_DIR)

check:
	! grep -r "[[:blank:]]$$" HAP PAL External Tests Applications

%.debug:
	$(DEBUGGER) $(basename $@)

docs:
	Tools/generate_api_doc.sh

DEST ?= Output
install:
	@rm -rf $(DEST)/Applications/
	@$(foreach protocol,$(PROTOCOLS),mkdir -p $(DEST)/Applications/$(protocol);)
	@$(foreach protocol,$(PROTOCOLS),cp $(OUTPUT_DIR)/$(BUILD_TYPE)/$(protocol)/Applications/*.$(CRYPTO) $(DEST)/Applications/$(protocol) 2>/dev/null | true;)
	@$(foreach protocol,$(PROTOCOLS),cp $(OUTPUT_DIR)/$(BUILD_TYPE)/$(protocol)/Applications/*.a $(DEST)/Applications/$(protocol) 2>/dev/null | true;)
	@$(foreach protocol,$(PROTOCOLS),cp $(OUTPUT_DIR)/$(BUILD_TYPE)/$(protocol)/Applications/*.$(SO_EXT) $(DEST)/Applications/$(protocol) 2>/dev/null | true;)
	@mkdir -p $(DEST)/Applications/include
	@cp -f Applications/ADK.h $(DEST)/Applications/include
	@echo Applications installed in $(DEST)/Applications/
	@rm -rf $(DEST)/Tools
	@mkdir -p $(DEST)/Tools
	@cp $(OUTPUT_DIR)/$(BUILD_TYPE)/Tools/*.$(CRYPTO) $(DEST)/Tools 2>/dev/null | true
	@cp $(OUTPUT_DIR)/$(BUILD_TYPE)/Tools/JLINK/{jlink,libjlinkarm.dylib,libjlinkarm.so} $(DEST)/Tools 2>/dev/null | true
	@echo Tools installed in $(DEST)/Tools

# Include test makefile
-include Build/Makefile.Test

# Include build tools makefile
-include Build/Makefile.Tools

# Import in all the compiler-generated dep files (to know which .c files depend on which .h files)
-include $(DEPS)
