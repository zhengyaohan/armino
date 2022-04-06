CFLAGS = -O2 -std=c++14

JLINK_MAJOR_VERSION = 6
LDFLAGS_JLINK :=
JLINK_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
OUTPUT_DIR ?= Output
HOST := $(shell uname)

ifeq ($(HOST),Darwin)
DLL_EXT := dylib
DLL = jlinkarm.6.70.7.$(DLL_EXT)
JLINK_SYSTEM_PATH ?= /Applications/SEGGER/JLink/lib$(DLL)
JLINK_DOWNLOAD_LINK=https://www.segger.com/downloads/jlink/JLink_MacOSX_V670g.pkg
else
DLL_EXT := so
DLL = jlinkarm.$(DLL_EXT).6.70.7
JLINK_SYSTEM_PATH ?= /usr/local/lib/lib$(DLL)
JLINK_DOWNLOAD_LINK=https://www.segger.com/downloads/jlink/JLink_Linux_V670g_x86_64.tgz
endif

all: $(OUTPUT_DIR)/jlink

$(OUTPUT_DIR)/libjlinkarm.$(DLL_EXT):
	@mkdir -p $(OUTPUT_DIR)
ifneq ("$(wildcard $(JLINK_DIR)/DLL/lib$(DLL))","")
    # Check if there is a local copy of the library available
	cp $(JLINK_DIR)/DLL/lib$(DLL) $(OUTPUT_DIR)/libjlinkarm.$(DLL_EXT)
else ifneq ("$(wildcard $(JLINK_SYSTEM_PATH))","")
	# Check if JLINK library is available on system
	cp $(JLINK_SYSTEM_PATH) $(OUTPUT_DIR)/libjlinkarm.$(DLL_EXT)
else
    # Throw an error if we couldn't find the JLINK library
    $(error Missing JLINK dependency. Please download and install $(JLINK_DOWNLOAD_LINK))
endif
ifeq ($(HOST),Darwin)
	install_name_tool -id "@loader_path/libjlinkarm.$(DLL_EXT)" $(OUTPUT_DIR)/libjlinkarm.$(DLL_EXT)
else
	ln -s libjlinkarm.$(DLL_EXT) $(OUTPUT_DIR)/libjlinkarm.$(DLL_EXT).$(JLINK_MAJOR_VERSION)
endif

$(OUTPUT_DIR)/%.o: $(JLINK_DIR)/Source/%.cpp $(wildcard $(JLINK_DIR)/Source/*.h)
	@mkdir -p $(OUTPUT_DIR)
	clang++ $(CFLAGS) -c -o $@ $<

$(OUTPUT_DIR)/jlink: $(addprefix $(OUTPUT_DIR)/,JLink.o TCPServer.o Loader.o GDBServer.o Main.o) $(OUTPUT_DIR)/libjlinkarm.$(DLL_EXT)
ifeq ($(HOST),Darwin)
	clang++ -o $@ $^ -L$(OUTPUT_DIR) -ljlinkarm
else
	clang++ -o $@ $^ -L$(OUTPUT_DIR) -ljlinkarm -Wl,-rpath,$(realpath $(OUTPUT_DIR))
endif

clean:
	rm -rf Source/*~ $(OUTPUT_DIR)/
