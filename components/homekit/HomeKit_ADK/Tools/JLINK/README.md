Cortex-M development tools using J-Link
=======================================

This repository contains tools for debugging and test automation on ARM
based platforms using J-Link.

#### Usage
```
usage: jlink -list | --flash | --run | --gdbserver | --help | --version
       [--select INDEX] [--log] [--realtime] [--resume] [--port PORT] [files ...]

Actions:

   --list                 list attached devices
   --flash                flash an application (you can provide multiple non-overlapping
                          IHEX or ELF binaries)
   --run                  flash and run an application (accepts file arguments like --flash)
   --gdbserver            launch as GDB server and wait for incoming connections
   --help                 print this help text
   --version              print program build date

General options:

   --select INDEX         select device to use (by index)
   --log                  print detailed log information while running

Options when running an application:

   --realtime             don't pause target when terminal output is overflowing
   --resume               resume execution when we exit (instead of leaving the target halted)

Options for GDB server:

   --port PORT            Listen on PORT (default: 8888)

Examples:

   jlink --list
   jlink --select 1 --log --flash s140_nrf52_6.1.1_softdevice.hex HAPBase+IntTests.Oberon
   jlink --port 8080 --realtime --gdbserver
```

jlinkarm.h
----------

SEGGER does not publicly distribute the header files that go along with the
J-Link DLLs. *jlinkarm.h* was derived from: https://pypi.org/project/pylink-square/
