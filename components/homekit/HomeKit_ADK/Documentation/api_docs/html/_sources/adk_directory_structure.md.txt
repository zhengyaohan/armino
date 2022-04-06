ADK Directory Structure
=======================

The previous section explained how the software of a HomeKit accessory works at runtime. Key elements of this software,
in particular the HAP Library, are provided by Apple in the ADK, or are redistributed by chipset vendors in their
HomeKit compatible SDKs.

## Applications
The Applications subdirectory contains source code of typical accessories. A sample application is comprised of several
components:
- *Applications/SampleApp* - Collection of source files that defines the accessory.
- *Applications/Main.c* - Common application entry point, contains main().
- *Applications/Common/DB* -  Collection of source files defining HAP services and characteristics such that these
services/characteristics can be used across different applications.
- *Applications/Common/Helper* - Collection of source files that implement default handlers for HAP services as well
as any code that can be shared across different applications.
- *Applications/Common/Platform* - Application specific logic that is platform dependent but not generic enough
warranting a fully developed PAL API.

## HAP
The HAP implementation is provided in the HAP subdirectory. It is not expected for vendors to have to change HAP API or
its implementation in this subdirectory.

## PAL
The Platform Abstraction Layer implementation is provided in the PAL subdirectory.

The `HAPPlatform.h` file is an umbrella header file for the header files of the individual PAL module APIs, namely
`HAPPlatform<Module_Name>.h`. A PAL developer must provide implementations for all `HAPPlatform<Module_Name>.h` files.
The other header files are helpers that need not be modified:
- *HAPAssert.h* that the HAP Library uses for checking assertions, e.g., through precondition checks.
- *HAPBase.h* defines common base types needed in different PAL modules and in HAP.h.
- *HAPBase+CompilerAbstraction.h* makes it possible to support a variety of compilers.
- *HAPLog.h* defines the logging levels supported by the ADK for the HAP Library.

A header file may contain dependencies to one or several lower layer header files, but not vice versa. For example,
`HAPPlatform.h` includes `HAPPlatformTimer.h`, `HAPPlatformRunLoop.h`, `HAPPlatformAbort.h`, etc., but
`HAPPlatformAbort.h` must not include e.g. `HAPAssert.h`:
- *HAPPlatform.h*
- *HAPPlatform<Module_Name>.h*
- *HAPBase.h*
- *HAPAssert.h*
- *HAPLog.h*
- *HAPPlatformAbort.h*
- *HAPBase+CompilerAbstraction.h*

The files to be implemented are listed above in bold. More information about each of them is provided in the
[PAL Modules](_api_docs/dir_PAL). A platform developer should include `HAPPlatform.h` in order to include
all header file dependencies.
