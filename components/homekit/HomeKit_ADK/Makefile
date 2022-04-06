export

STEPS := apps tests all static shared check info tools docs install %.debug
.PHONY: $(STEPS) %.debug shell docker lint lint-changed clean

CWD := $(shell pwd)
HOST := $(shell uname)

JOBS ?=
ifeq ($(HOST),Darwin)
  JOBS := $(or $(JOBS),$(shell sysctl -n hw.logicalcpu))
else
  JOBS := $(or $(JOBS),$(shell nproc))
endif

OUTPUT_DIR := Output
TARGET_NO_DEFAULT := $(TARGET)
TARGET ?= $(HOST)
BUILD_TYPE ?= Debug
SUB_MAKE := make -f Build/Makefile -j $(JOBS)
DOCKER_BUILDKIT ?= 0
DOCKER_EXE := DOCKER_BUILDKIT=$(DOCKER_BUILDKIT) docker
DOCKER ?= 1

DOCKER_ENVS := \
  -e APPS \
  -e ASAN \
  -e BUILD_TYPE \
  -e CCACHE_DIR=/build/.ccache_$(TARGET) \
  -e CRYPTO \
  -e ENABLE_BUFFER_LOGS \
  -e ENABLE_HAP_TESTING \
  -e HAP_ACCESSORY_METRICS \
  -e HAP_ACCESSORY_REACHABILITY \
  -e HAP_ADAPTIVE_LIGHT \
  -e HAP_DIAGNOSTICS_MANUFACTURER \
  -e HAP_DIAGNOSTICS_SERVICE \
  -e HAP_DYNAMIC_MEMORY_ALLOCATION \
  -e HAP_FIRMWARE_UPDATE \
  -e HAP_HDS_RX_BUFFER_LOG_MAX_BYTES \
  -e HAP_HDS_TRANSPORT_OVER_HAP \
  -e HAP_LOCK_ENC \
  -e HAP_LOG_MESSAGE_MAX_BYTES \
  -e HAP_METRICS_MAX_STORED_EVENTS \
  -e HAP_THREAD_DECOMMISSION_ON_UNPAIR \
  -e HAP_VIDEODOORBELL_OPERATING_STATE \
  -e HAP_VIDEODOORBELL_SILENT_MODE \
  -e HAP_WIFI_RECONFIGURATION \
  -e HOMEKITSTORE_DIR \
  -e HOST \
  -e LOG_LEVEL \
  -e PROFILE \
  -e PROTOCOLS \
  -e SENSOR \
  -e TARGET \
  -e TESTS \
  -e USE_ACCESS_CODE \
  -e USE_ACCESSORY_METRICS \
  -e USE_BATTERY_POWERED_CAMERA_RECORDER \
  -e USE_BLE \
  -e USE_CCACHE \
  -e USE_COLOR_TEMPERATURE \
  -e USE_DIAGNOSTICS_TEXT_FORMAT \
  -e USE_DISPLAY \
  -e USE_HW_AUTH \
  -e USE_MTD \
  -e USE_NFC \
  -e USE_NFC_ACCESS \
  -e USE_PORTRAIT_MODE \
  -e USE_STATIC_COMMISSIONING \
  -e USE_THREAD_HOSTNAME \
  -e USE_TOKEN_AUTH \
  -e USE_WAC \

DOCKERFILE := Build/Docker/Dockerfile
TAG_IMAGE := -t dev-test/linuxadk-base:latest
ifeq ($(DOCKER_BUILDKIT),1)
  MAKE_DOCKER = rm -rf iid && $(DOCKER_EXE) build $(TAG_IMAGE) -f $(DOCKERFILE) Build/Docker --iidfile iid && cat iid | cut -d ':' -f 2
else
  MAKE_DOCKER = $(DOCKER_EXE) build $(TAG_IMAGE) -f $(DOCKERFILE) Build/Docker | tee /dev/stderr | grep "Successfully built" | cut -d ' ' -f 3
endif
RUN = $(DOCKER_EXE) run \
  $(DOCKER_ENVS) \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  --mount type=bind,source="$(CWD)",target=/build \
  --rm \
  -i $(DOCKER_TTY_OPTION) `$(MAKE_DOCKER)`

ifeq ($(HOST)$(TARGET),LinuxDarwin)
$(error Can't build $(TARGET) on $(HOST).)
endif
ifneq ($(TARGET),Darwin)
	ifneq (,$(wildcard /.dockerenv))
		# If we are already running inside docker
		SUB_MAKE := $(SUB_MAKE)
	else ifeq ($(DOCKER),0)
		SUB_MAKE := $(SUB_MAKE)
	else
		# Else run make inside docker
		SUB_MAKE := $(RUN) $(SUB_MAKE)
	endif
endif

define make_target
  $(1):
	@$(2) $$@

endef

$(eval $(foreach step,$(STEPS),$(call make_target,$(step),$(SUB_MAKE) PAL=$(TARGET))))

shell:ENABLE_TTY?=1
ifeq ($(ENABLE_TTY),0)
    DOCKER_TTY_OPTION :=
else
    DOCKER_TTY_OPTION := -t
endif
shell:
	@$(RUN) bash

clean:
ifneq ($(TARGET_NO_DEFAULT),)
	$(info Cleaning output folder for target: $(TARGET))
	@$(SUB_MAKE) PAL=$(TARGET) clean
else
	$(info Cleaning entire output folder)
	@rm -rf $(OUTPUT_DIR)
endif

docker:
	@$(MAKE_DOCKER)

# Lint (and correct) all files.
lint:
	@Tools/linters/lint.sh -c

# Lint (and correct) all currently-changed files (according to git).
lint-changed:
	@git diff-index --name-only HEAD \
	  | xargs printf -- '-f\0%s\0' \
 	  | xargs -0 -- Tools/linters/lint.sh -c

