// Disclaimer: IMPORTANT: This Apple software is supplied to you, by Apple Inc. ("Apple"), in your
// capacity as a current, and in good standing, Licensee in the MFi Licensing Program. Use of this
// Apple software is governed by and subject to the terms and conditions of your MFi License,
// including, but not limited to, the restrictions specified in the provision entitled "Public
// Software", and is further subject to your agreement to the following additional terms, and your
// agreement that the use, installation, modification or redistribution of this Apple software
// constitutes acceptance of these additional terms. If you do not agree with these additional terms,
// you may not use, install, modify or redistribute this Apple software.
//
// Subject to all of these terms and in consideration of your agreement to abide by them, Apple grants
// you, for as long as you are a current and in good-standing MFi Licensee, a personal, non-exclusive
// license, under Apple's copyrights in this Apple software (the "Apple Software"), to use,
// reproduce, and modify the Apple Software in source form, and to use, reproduce, modify, and
// redistribute the Apple Software, with or without modifications, in binary form, in each of the
// foregoing cases to the extent necessary to develop and/or manufacture "Proposed Products" and
// "Licensed Products" in accordance with the terms of your MFi License. While you may not
// redistribute the Apple Software in source form, should you redistribute the Apple Software in binary
// form, you must retain this notice and the following text and disclaimers in all such redistributions
// of the Apple Software. Neither the name, trademarks, service marks, or logos of Apple Inc. may be
// used to endorse or promote products derived from the Apple Software without specific prior written
// permission from Apple. Except as expressly stated in this notice, no other rights or licenses,
// express or implied, are granted by Apple herein, including but not limited to any patent rights that
// may be infringed by your derivative works or by other works in which the Apple Software may be
// incorporated. Apple may terminate this license to the Apple Software by removing it from the list
// of Licensed Technology in the MFi License, or otherwise in accordance with the terms of such MFi License.
//
// Unless you explicitly state otherwise, if you provide any ideas, suggestions, recommendations, bug
// fixes or enhancements to Apple in connection with this software ("Feedback"), you hereby grant to
// Apple a non-exclusive, fully paid-up, perpetual, irrevocable, worldwide license to make, use,
// reproduce, incorporate, modify, display, perform, sell, make or have made derivative works of,
// distribute (directly or indirectly) and sublicense, such Feedback in connection with Apple products
// and services. Providing this Feedback is voluntary, but if you do provide Feedback to Apple, you
// acknowledge and agree that Apple may exercise the license granted above without the payment of
// royalties or further consideration to Participant.

// The Apple Software is provided by Apple on an "AS IS" basis. APPLE MAKES NO WARRANTIES, EXPRESS OR
// IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR
// IN COMBINATION WITH YOUR PRODUCTS.
//
// IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION
// AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
// (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (C) 2015-2020 Apple Inc. All Rights Reserved.

#ifndef __jlinkarm_h__
#define __jlinkarm_h__

#include <stddef.h>
#include <stdint.h>

enum class JLinkGlobalErrors {
    UNSPECIFIED_ERROR = -1,
    EMU_NO_CONNECTION = -256,
    EMU_COMM_ERROR = -257,
    DLL_NOT_OPEN = -258,
    VCC_FAILURE = -259,
    INVALID_HANDLE = -260,
    NO_CPU_FOUND = -261,
    EMU_FEATURE_UNSUPPORTED = -262,
    EMU_NO_MEMORY = -263,
    TIF_STATUS_ERROR = -264,
    FLASH_PROG_COMPARE_FAILED = -265,
    FLASH_PROG_PROGRAM_FAILED = -266,
    FLASH_PROG_VERIFY_FAILED = -267,
    OPEN_FILE_FAILED = -268,
    UNKNOWN_FILE_FORMAT = -269,
    WRITE_TARGET_MEMORY_FAILED = -270,
    DEVICE_FEATURE_NOT_SUPPORTED = -271,
    WRONG_USER_CONFIG = -272,
    NO_TARGET_DEVICE_SELECTED = -273,
    CPU_IN_LOW_POWER_MODE = -274,
};

enum class JLinkEraseErrors {
    ILLEGAL_COMMAND = -5,
};

enum class JLinkFlashErrors {
    COMPARE_ERROR = -2,
    PROGRAM_ERASE_ERROR = -3,
    VERIFICATION_ERROR = -4,
};

enum class JLinkWriteErrors {
    ZONE_NOT_FOUND_ERROR = -5,
};

enum class JLinkReadErrors {
    ZONE_NOT_FOUND_ERROR = -5,
};

enum class JLinkDataErrors {
    ERROR_UNKNOWN = (int) 0x80000000,
    ERROR_NO_MORE_EVENTS = (int) 0x80000001,
    ERROR_NO_MORE_ADDR_COMP = (int) 0x80000002,
    ERROR_NO_MORE_DATA_COMP = (int) 0x80000004,
    ERROR_INVALID_ADDR_MASK = (int) 0x80000020,
    ERROR_INVALID_DATA_MASK = (int) 0x80000040,
    ERROR_INVALID_ACCESS_MASK = (int) 0x80000080,
};

enum class JLinkRTTErrors {
    RTT_ERROR_CONTROL_BLOCK_NOT_FOUND = -2,
};

enum class JLinkHost {
    USB = (1 << 0),
    IP = (1 << 1),
    USB_OR_IP = USB | IP,
};

enum class JLinkInterface {
    JTAG = 0,
    SWD = 1,
    FINE = 3,
    ICSP = 4,
    SPI = 5,
    C2 = 6,
};

/*
  NORMAL: default reset strategy, does whatever is best to reset.
  CORE: only the core is reset via the ``VECTRESET`` bit.
  RESETPIN: pulls the reset pin low to reset the core and peripherals.
  CONNECT_UNDER_RESET: J-Link connects to target while keeping reset
                       active.  This is recommended for STM32 devices.
  HALT_AFTER_BTL: halt the core after the bootloader is executed.
  HALT_BEFORE_BTL: halt the core before the bootloader is executed.
  KINETIS: performs a normal reset, but also disables the watchdog.
  ADI_HALT_AFTER_KERNEL: sets the ``SYSRESETREQ`` bit in the ``AIRCR`` in
                         order to reset the device.
  CORE_AND_PERIPHERALS: sets the ``SYSRESETREQ`` bit in the ``AIRCR``, and
                        the ``VC_CORERESET`` bit in the ``DEMCR`` to make
                        sure that the CPU is halted immediately after reset.
  LPC1200: reset for LPC1200 devices.
  S3FN60D: reset for Samsung S3FN60D devices.
  Note:
  Please see the J-Link SEGGER Documentation, UM8001, for full information
  about the different reset strategies.
*/
enum class JLinkResetStrategyCortexM3 {
    NORMAL = 0,
    CORE = 1,
    RESETPIN = 2,
    CONNECT_UNDER_RESET = 3,
    HALT_AFTER_BTL = 4,
    HALT_BEFORE_BTL = 5,
    KINETIS = 6,
    ADI_HALT_AFTER_KERNEL = 7,
    CORE_AND_PERIPHERALS = 8,
    LPC1200 = 9,
    S3FN60D = 10,
};

enum class JLinkCore {
    NONE = 0x00000000,
    ANY = (int) 0xFFFFFFFF,
    CORTEX_M1 = 0x010000FF,
    COLDFIRE = 0x02FFFFFF,
    CORTEX_M3 = 0x030000FF,
    CORTEX_M3_R1P0 = 0x03000010,
    CORTEX_M3_R1P1 = 0x03000011,
    CORTEX_M3_R2P0 = 0x03000020,
    SIM = 0x04FFFFFF,
    XSCALE = 0x05FFFFFF,
    CORTEX_M0 = 0x060000FF,
    CORTEX_M_V8BASEL = 0x060100FF,
    ARM7 = 0x07FFFFFF,
    ARM7TDMI = 0x070000FF,
    ARM7TDMI_R3 = 0x0700003F,
    ARM7TDMI_R4 = 0x0700004F,
    ARM7TDMI_S = 0x070001FF,
    ARM7TDMI_S_R3 = 0x0700013F,
    ARM7TDMI_S_R4 = 0x0700014F,
    CORTEX_A8 = 0x080000FF,
    CORTEX_A7 = 0x080800FF,
    CORTEX_A9 = 0x080900FF,
    CORTEX_A12 = 0x080A00FF,
    CORTEX_A15 = 0x080B00FF,
    CORTEX_A17 = 0x080C00FF,
    ARM9 = 0x09FFFFFF,
    ARM9TDMI_S = 0x090001FF,
    ARM920T = 0x092000FF,
    ARM922T = 0x092200FF,
    ARM926EJ_S = 0x092601FF,
    ARM946E_S = 0x094601FF,
    ARM966E_S = 0x096601FF,
    ARM968E_S = 0x096801FF,
    ARM11 = 0x0BFFFFFF,
    ARM1136 = 0x0B36FFFF,
    ARM1136J = 0x0B3602FF,
    ARM1136J_S = 0x0B3603FF,
    ARM1136JF = 0x0B3606FF,
    ARM1136JF_S = 0x0B3607FF,
    ARM1156 = 0x0B56FFFF,
    ARM1176 = 0x0B76FFFF,
    ARM1176J = 0x0B7602FF,
    ARM1176J_S = 0x0B7603FF,
    ARM1176JF = 0x0B7606FF,
    ARM1176JF_S = 0x0B7607FF,
    CORTEX_R4 = 0x0C0000FF,
    CORTEX_R5 = 0x0C0100FF,
    RX = 0x0DFFFFFF,
    RX610 = 0x0D00FFFF,
    RX62N = 0x0D01FFFF,
    RX62T = 0x0D02FFFF,
    RX63N = 0x0D03FFFF,
    RX630 = 0x0D04FFFF,
    RX63T = 0x0D05FFFF,
    RX621 = 0x0D06FFFF,
    RX62G = 0x0D07FFFF,
    RX631 = 0x0D08FFFF,
    RX210 = 0x0D10FFFF,
    RX21A = 0x0D11FFFF,
    RX220 = 0x0D12FFFF,
    RX230 = 0x0D13FFFF,
    RX231 = 0x0D14FFFF,
    RX23T = 0x0D15FFFF,
    RX111 = 0x0D20FFFF,
    RX110 = 0x0D21FFFF,
    RX113 = 0x0D22FFFF,
    RX64M = 0x0D30FFFF,
    RX71M = 0x0D31FFFF,
    CORTEX_M4 = 0x0E0000FF,
    CORTEX_M7 = 0x0E0100FF,
    CORTEX_M_V8MAINL = 0x0E0200FF,
    CORTEX_A5 = 0x0F0000FF,
    POWER_PC = 0x10FFFFFF,
    POWER_PC_N1 = 0x10FF00FF,
    POWER_PC_N2 = 0x10FF01FF,
    MIPS = 0x11FFFFFF,
    MIPS_M4K = 0x1100FFFF,
    MIPS_MICROAPTIV = 0x1101FFFF,
    EFM8_UNSPEC = 0x12FFFFFF,
    CIP51 = 0x1200FFFF,
};

enum class JLinkDeviceFamily {
    AUTO = 0,
    CORTEX_M1 = 1,
    COLDFIRE = 2,
    CORTEX_M3 = 3,
    SIMULATOR = 4,
    XSCALE = 5,
    CORTEX_M0 = 6,
    ARM7 = 7,
    CORTEX_A8 = 8,
    CORTEX_A9 = 8,
    ARM9 = 9,
    ARM10 = 10,
    ARM11 = 11,
    CORTEX_R4 = 12,
    RX = 13,
    CORTEX_M4 = 14,
    CORTEX_A5 = 15,
    POWERPC = 16,
    MIPS = 17,
    EFM8 = 18,
    ANY = 255,
};

enum class JLinkFlags {
    GO_OVERSTEP_BP = (1 << 0),

    DLG_BUTTON_YES = (1 << 0),
    DLG_BUTTON_NO = (1 << 1),
    DLG_BUTTON_OK = (1 << 2),
    DLG_BUTTON_CANCEL = (1 << 3),

    HW_PIN_STATUS_LOW = 0,
    HW_PIN_STATUS_HIGH = 1,
    HW_PIN_STATUS_UNKNOWN = 255,
};

enum class JLinkSWOInterfaces {
    UART = 0,
    MANCHESTER = 1, // DO NOT USE
};

enum class JLinkSWOCommands {
    START = 0,
    STOP = 1,
    FLUSH = 2,
    GET_SPEED_INFO = 3,
    GET_NUM_BYTES = 10,
    SET_BUFFERSIZE_HOST = 20,
    SET_BUFFERSIZE_EMU = 21,
};

enum class JLinkCPUCapabilities {
    READ_MEMORY = (1 << 1),
    WRITE_MEMORY = (1 << 2),
    READ_REGISTERS = (1 << 3),
    WRITE_REGISTERS = (1 << 4),
    GO = (1 << 5),
    STEP = (1 << 6),
    HALT = (1 << 7),
    IS_HALTED = (1 << 8),
    RESET = (1 << 9),
    RUN_STOP = (1 << 10),
    TERMINAL = (1 << 11),
    DCC = (1 << 14),
    HSS = (1 << 15),
};

/*
DBGRQ: CPU has been halted because DBGRQ signal asserted.
CODE_BREAKPOINT: CPU has been halted because of code breakpoint match.
DATA_BREAKPOINT: CPU has been halted because of data breakpoint match.
VECTOR_CATCH: CPU has been halted because of vector catch.
*/
enum class JLinkHaltReasonType {
    DBGRQ = 0,
    CODE_BREAKPOINT = 1,
    DATA_BREAKPOINT = 2,
    VECTOR_CATCH = 3,
};

/*
CORE_RESET: The CPU core reset.
MEM_ERROR: A memory management error occurred.
COPROCESSOR_ERROR: Usage fault error accessing the Coprocessor.
CHECK_ERROR: Usage fault error on enabled check.
STATE_ERROR: Usage fault state error.
BUS_ERROR: Normal bus error.
INT_ERROR: Interrupt or exception service error.
HARD_ERROR: Hard fault error.
*/
enum class JLinkVectorCatchCortexM3 {
    CORE_RESET = (1 << 0),
    MEM_ERROR = (1 << 4),
    COPROCESSOR_ERROR = (1 << 5),
    CHECK_ERROR = (1 << 6),
    STATE_ERROR = (1 << 7),
    BUS_ERROR = (1 << 8),
    INT_ERROR = (1 << 9),
    HARD_ERROR = (1 << 10),
};

/*
SW_RAM: Software breakpoint located in RAM.
SW_FLASH: Software breakpoint located in flash.
SW: Software breakpoint located in RAM or flash.
HW: Hardware breakpoint.
ANY: Allows specifying any time of breakpoint.
ARM: Breakpoint in ARM mode (only available on ARM 7/9 cores).
THUMB: Breakpoint in THUMB mode (only available on ARM 7/9 cores).
*/
enum class JLinkBreakpoint {
    SW_RAM = (1 << 4),
    SW_FLASH = (1 << 5),
    SW = 0x000000F0,
    HW = (int) 0xFFFFFF00,
    ANY = (int) 0xFFFFFFF0,
    ARM = (1 << 0),
    THUMB = (2 << 0),
};

/*
HARD: Hardware breakpoint using a breakpoint unit.
SOFT: Software breakpoint using a breakpoint instruction.
PENDING: Breakpoint has not been set yet.
FLASH: Breakpoint set in flash.
*/
enum class JLinkBreakpointImplementation {
    HARD = (1 << 0),
    SOFT = (1 << 1),
    PENDING = (1 << 2),
    FLASH = (1 << 4),
};

enum class JLinkEventTypes {
    BREAKPOINT = (1 << 0),
};

/*
READ: specifies to monitor read accesses.
WRITE: specifies to monitor write accesses.
PRIVILEGED: specifies to monitor privileged accesses.
SIZE_8BIT: specifies to monitor an 8-bit access width.
SIZE_16BIT: specifies to monitor an 16-bit access width.
SIZE_32BIT: specifies to monitor an 32-bit access width.
*/
enum class JLinkAccessFlags {
    READ = (0 << 0),
    WRITE = (1 << 0),
    PRIV = (1 << 4),
    SIZE_8BIT = (0 << 1),
    SIZE_16BIT = (1 << 1),
    SIZE_32BIT = (2 << 1),
};

/*
SIZE: specifies to not care about the access size of the event.
DIR: specifies to not care about the access direction of the event.
PRIV: specifies to not care about the access privilege of the event.
*/
enum class JLinkAccessMaskFlags {
    SIZE = (3 << 1),
    DIR = (1 << 0),
    PRIV = (1 << 4),
};

enum class JLinkStraceCommand {
    TRACE_EVENT_SET = 0,
    TRACE_EVENT_CLR = 1,
    TRACE_EVENT_CLR_ALL = 2,
    SET_BUFFER_SIZE = 3,
};

enum class JLinkStraceEvent {
    CODE_FETCH = 0,
    DATA_ACCESS = 1,
    DATA_LOAD = 2,
    DATA_STORE = 3,
};

enum class JLinkStraceOperation {
    TRACE_START = 0,
    TRACE_STOP = 1,
    TRACE_INCLUDE_RANGE = 2,
    TRACE_EXCLUDE_RANGE = 3,
};

enum class JLinkTraceSource {
    ETB = 0,
    ETM = 1,
    MTB = 2,
};

enum class JLinkTraceCommand {
    START = 0x0,
    STOP = 0x1,
    FLUSH = 0x2,
    GET_NUM_SAMPLES = 0x10,
    GET_CONF_CAPACITY = 0x11,
    SET_CAPACITY = 0x12,
    GET_MIN_CAPACITY = 0x13,
    GET_MAX_CAPACITY = 0x14,
    SET_FORMAT = 0x20,
    GET_FORMAT = 0x21,
    GET_NUM_REGIONS = 0x30,
    GET_REGION_PROPS = 0x31,
    GET_REGION_PROPS_EX = 0x32,
};

/*
FORMAT_4BIT: 4-bit data.
FORMAT_8BIT: 8-bit data.
FORMAT_16BIT: 16-bit data.
FORMAT_MULTIPLEXED: multiplexing on ETM / buffer link.
FORMAT_DEMULTIPLEXED: de-multiplexing on ETM / buffer link.
FORMAT_DOUBLE_EDGE: clock data on both ETM / buffer link edges.
FORMAT_ETM7_9: ETM7/ETM9 protocol.
FORMAT_ETM10: ETM10 protocol.
FORMAT_1BIT: 1-bit data.
FORMAT_2BIT: 2-bit data.
*/
enum class JLinkTraceFormat {
    FORMAT_4BIT = 0x1,
    FORMAT_8BIT = 0x2,
    FORMAT_16BIT = 0x4,
    FORMAT_MULTIPLEXED = 0x8,
    FORMAT_DEMULTIPLEXED = 0x10,
    FORMAT_DOUBLE_EDGE = 0x20,
    FORMAT_ETM7_9 = 0x40,
    FORMAT_ETM10 = 0x80,
    FORMAT_1BIT = 0x100,
    FORMAT_2BIT = 0x200,
};

enum class JLinkROMTable {
    NONE = 0x100,
    ETM = 0x101,
    MTB = 0x102,
    TPIU = 0x103,
    ITM = 0x104,
    DWT = 0x105,
    FPB = 0x106,
    NVIC = 0x107,
    TMC = 0x108,
    TF = 0x109,
    PTM = 0x10A,
    ETB = 0x10B,
    DBG = 0x10C,
    APBAP = 0x10D,
    AHBAP = 0x10E,
    SECURE = 0x10F,
};

enum class JLinkRTTCommand {
    START = 0,
    STOP = 1,
    GETDESC = 2,
    GETNUMBUF = 3,
    GETSTAT = 4,
};

enum class JLinkRTTDirection {
    UP = 0,
    DOWN = 1,
};

struct JLinkConnectInfo {
    uint32_t SerialNumber;
    uint8_t Connection;
    uint32_t USBAddr;
    uint8_t aIPAddr[16];
    int Time;
    uint64_t Time_us;
    uint32_t HWVersion;
    uint8_t abMACAddr[6];
    char acProduct[32];
    char acNickname[32];
    char acFWString[112];
    int8_t IsDHCPAssignedIP;
    int8_t IsDHCPAssignedIPIsValid;
    uint8_t NumIPConnections;
    uint8_t NumIPConnectionsIsValid;
    uint8_t aPadding[34];
};

struct JLinkFlashArea {
    uint32_t Addr;
    uint32_t Size;
};

struct JLinkRAMArea {
    uint32_t Addr;
    uint32_t Size;
};

/*
SizeOfStruct: Size of the struct (DO NOT CHANGE).
sName: name of the device.
CoreId: core identifier of the device.
FlashAddr: base address of the internal flash of the device.
RAMAddr: base address of the internal RAM of the device.
EndianMode: the endian mode of the device (0 -> only little endian,
1 -> only big endian, 2 -> both).
FlashSize: total flash size in bytes.
RAMSize: total RAM size in bytes.
sManu: device manufacturer.
aFlashArea: a list of ``JLinkFlashArea`` instances.
aRamArea: a list of ``JLinkRAMArea`` instances.
Core: CPU core.
*/
struct JLinkDeviceInfo {
    uint32_t SizeOfStruct;
    const char* sName;
    uint32_t CoreId;
    uint32_t FlashAddr;
    uint32_t RAMAddr;
    uint8_t EndianMode;
    uint32_t FlashSize;
    uint32_t RAMSize;
    const char* sManu;
    JLinkFlashArea aFlashArea[32];
    JLinkRAMArea aRAMArea[32];
    uint32_t Core;
};

struct JLinkHardwareStatus {
    uint32_t VTarget;
    uint8_t tck;
    uint8_t tdi;
    uint8_t tdo;
    uint8_t tms;
    uint8_t tres;
    uint8_t trst;
};

struct JLinkGPIODescriptor {
    char acName[32];
    uint32_t Caps;
};

/*
sName: initials of the memory zone.
sDesc: name of the memory zone.
VirtAddr: start address of the virtual address space of the memory zone.
abDummy: reserved for future use.
*/
struct JLinkMemoryZone {
    const char* sName;
    const char* sDesc;
    uint64_t VirtAddr;
    uint8_t abDummy[16];
};

/*
SizeOfStruct: the size of this structure.
BaseFreq: Base frequency (in HZ) used to calculate supported speeds.
MinDiv: minimum divider allowed to divide the base frequency.
SupportAdaptive: ``1`` if emulator supports adaptive clocking, otherwise
*/
struct JLinkSpeedInfo {
    uint32_t SizeOfStruct;
    uint32_t BaseFreq;
    uint16_t MinDiv;
    uint16_t SupportAdaptive;
};

/*
SizeofStruct: size of the structure.
Interface: the interface type used for SWO.
Speed: the frequency used for SWO communication in Hz.
*/
struct JLinkSWOStartInfo {
    uint32_t SizeofStruct;
    uint32_t Interface;
    uint32_t Speed;
};

/*
SizeofStruct: size of the structure.
Interface: interface type for the speed information.
BaseFreq: base frequency (Hz) used to calculate supported SWO speeds.
MinDiv: minimum divider allowed to divide the base frequency.
MaxDiv: maximum divider allowed to divide the base frequency.
MinPrescale: minimum prescaler allowed to adjust the base frequency.
MaxPrescale: maximum prescaler allowed to adjust the base frequency.
*/
struct JLinkSWOSpeedInfo {
    uint32_t SizeofStruct;
    uint32_t Interface;
    uint32_t BaseFreq;
    uint32_t MinDiv;
    uint32_t MaxDiv;
    uint32_t MinPrescale;
    uint32_t MaxPrescale;
};

/*
HaltReason: reason why the CPU stopped.
Index: if cause of CPU stop was a code/data breakpoint, this identifies
       the index of the code/data breakpoint unit which causes the CPU to
       stop, otherwise it is ``-1``.
*/
struct JLinkMOEInfo {
    uint32_t HaltReason;
    int Index;
};

/*
SizeOfStruct: the size of the structure (this should not be modified).
Handle: breakpoint handle.
Addr: address of where the breakpoint has been set.
Type: type flags which were specified when the breakpoint was created.
ImpFlags: describes the current state of the breakpoint.
UseCnt: describes how often the breakpoint is set at the same address.
*/
struct JLinkHaltReasons {
    uint32_t SizeOfStruct;
    uint32_t Handle;
    uint32_t Addr;
    uint32_t Type;
    uint32_t ImpFlags;
    uint32_t UseCnt;
};

/*
SizeOfStruct: the size of the structure (this should not be modified).
Handle: breakpoint handle.
Addr: address of where the breakpoint has been set.
Type: type flags which were specified when the breakpoint was created.
ImpFlags: describes the current state of the breakpoint.
UseCnt: describes how often the breakpoint is set at the same address.
*/
struct JLinkBreakpointInfo {
    uint32_t SizeOfStruct;
    uint32_t Handle;
    uint32_t Addr;
    uint32_t Type;
    uint32_t ImpFlags;
    uint32_t UseCnt;
};

/*
SizeOfStruct: the size of the structure (this should not be modified).
Type: the type of the data event (this should not be modified).
Addr: the address on which the watchpoint was set
AddrMask: the address mask used for comparision.
Data: the data on which the watchpoint has been set.
DataMask: the data mask used for comparision.
Access: the control data on which the event has been set.
AccessMask: the control mask used for comparison.
*/
struct JLinkDataEvent {
    int SizeOfStruct;
    int Type;
    uint32_t Addr;
    uint32_t AddrMask;
    uint32_t Data;
    uint32_t DataMask;
    uint8_t Access;
    uint8_t AccessMask;
};

/*
SizeOfStruct: the size of the structure (this should not be modified).
Handle: the watchpoint handle.
Addr: the address the watchpoint was set at.
AddrMask: the address mask used for comparison.
Data: the data on which the watchpoint was set.
DataMask: the data mask used for comparision.
Ctrl: the control data on which the breakpoint was set.
CtrlMask: the control mask used for comparison.
WPUnit: the index of the watchpoint unit.
*/
struct JLinkWatchpointInfo {
    uint32_t SizeOfStruct;
    uint32_t Handle;
    uint32_t Addr;
    uint32_t AddrMask;
    uint32_t Data;
    uint32_t DataMask;
    uint32_t Ctrl;
    uint32_t CtrlMask;
    uint8_t WPUnit;
};

/*
SizeOfStruct: size of the structure.
Type: type of event.
Op: the STRACE operation to perform.
AccessSize: access width for trace events.
Reserved0: reserved.
Addr: specifies the load/store address for data.
Data: the data to be compared for the operation for data access events.
DataMask: bitmask for bits of data to omit in comparision for data access
          events.
AddrRangeSize: address range for range events.
*/
struct JLinkStraceEventInfo {
    uint32_t SizeOfStruct;
    uint8_t Type;
    uint8_t Op;
    uint8_t AccessSize;
    uint8_t Reserved0;
    uint64_t Addr;
    uint64_t Data;
    uint64_t DataMask;
    uint32_t AddrRangeSize;
};

/*
pipestat 0: instruction
pipestat 1: data_instruction
pipestat 2: non_instruction
pipestat 3: wait
pipestat 4: branch
pipestat 5: data_branch
pipestat 6: trigger
pipestat 7: trace_disabled
*/
struct JLinkTraceData {
    uint8_t PipeStat;
    uint8_t Sync;
    uint16_t Packet;
};

/*
SizeOfStruct: size of the structure.
RegionIndex: index of the region.
NumSamples: number of samples in the region.
Off: offset in the trace buffer.
RegionCnt: number of trace regions.
Dummy: unused.
Timestamp: timestamp of last event written to buffer.
*/
struct JLinkTraceRegion {
    uint32_t SizeOfStruct;
    uint32_t RegionIndex;
    uint32_t NumSamples;
    uint32_t Off;
    uint32_t RegionCnt;
    uint32_t Dummy;
    uint64_t Timestamp;
};

struct JLinkRTTBufferDescriptor {
    uint32_t Index;
    uint32_t Direction;
    char Name[32];
    uint32_t Size;
    uint32_t Flags;
};

typedef void (*JLinkLogCallback)(const char*);
typedef void (*JLinkProgressCallback)(const char*, const char*, int);

extern "C" {

int JLINKARM_GetDLLVersion();
int JLINKARM_HasError();
void JLINKARM_ClrError();
int JLINKARM_IsOpen();
int JLINKARM_EMU_IsConnected();
int JLINKARM_IsConnected();
void JLINKARM_EnableLog(JLinkLogCallback log);
void JLINKARM_EnableLogCom(JLinkLogCallback log);
void JLINKARM_SetErrorOutHandler(JLinkLogCallback log);
void JLINKARM_SetWarnOutHandler(JLinkLogCallback log);
int JLINKARM_EMU_GetNumDevices();
size_t JLINKARM_EMU_GetList(JLinkHost host, JLinkConnectInfo* array, size_t length);
int JLINKARM_DEVICE_GetInfo(int index, JLinkDeviceInfo* info);
int JLINKARM_EMU_SelectByUSBSN(uint32_t serial);
const char* JLINKARM_OpenEx(JLinkLogCallback log, JLinkLogCallback error);
void JLINKARM_Close();
int JLINKARM_Connect();
int JLINKARM_Test();
void JLINKARM_SetResetDelay(int ms);
int JLINKARM_SetInitRegsOnReset(int flag);
int JLINKARM_Reset();
void JLINKARM_ResetTRST();
void JLINKARM_EnableSoftBPs(int enable);
size_t JLINKARM_GetNumBPUnits(uint32_t flags);
size_t JLINKARM_GetNumBPs();
int JLINKARM_GetBPInfoEx(uint32_t index, JLinkBreakpointInfo* bp);
int JLINKARM_SetBPEx(uint32_t addr, uint32_t flags);
int JLINKARM_ClrBPEx(int handle);
size_t JLINKARM_GetNumWPUnits();
size_t JLINKARM_GetNumWPs();
int JLINKARM_GetWPInfoEx(uint32_t index, JLinkWatchpointInfo* wp);
int JLINKARM_SetDataEvent(JLinkDataEvent* ev, uint32_t* handle);
int JLINKARM_ClrDataEvent(uint32_t handle);
int JLINKARM_ExecCommand(const char* cmd, char* errmsg, size_t errmsg_maxsize);
int JLINKARM_UpdateFirmwareIfNewer();
int JLINKARM_CORE_GetFound();
int JLINKARM_CORESIGHT_Configure(const char* cfg);
size_t JLINKARM_GetSelDevice();
int JLINKARM_GetSpeed();
void JLINKARM_SetMaxSpeed();
void JLINKARM_TIF_GetAvailable(int* mask);
int JLINKARM_TIF_Select(JLinkInterface tif);
int JLINKARM_EMU_COM_IsSupported();
int JLINKARM_Halt();
int JLINKARM_IsHalted();
int JLINKARM_GetMOEs(JLinkMOEInfo* array, size_t max);
int JLINKARM_Step();
int JLINKARM_StepComposite();
int JLINKARM_GoEx(uint32_t num, uint32_t flags);
size_t JLINKARM_GetRegisterList(uint32_t* list, size_t max);
const char* JLINKARM_GetRegisterName(uint32_t reg);
int JLINKARM_ReadRegs(const uint32_t* regs, uint32_t* data, uint8_t* statuses, size_t count);
int JLINKARM_WriteRegs(const uint32_t* regs, const uint32_t* data, uint8_t* statuses, size_t count);
int JLINKARM_ReadMemEx(uint32_t addr, uint32_t bytes, uint8_t* data, size_t word_size);
int JLINKARM_WriteMemEx(uint32_t addr, uint32_t bytes, const uint8_t* data, size_t word_size);

int JLINK_RTTERMINAL_Control(uint32_t command, uint32_t* arg);
int JLINK_RTTERMINAL_Read(uint32_t index, char* buffer, uint32_t length);
int JLINK_EraseChip();
void JLINK_SetFlashProgProgressCallback(JLinkProgressCallback progress);
int JLINK_DownloadFile(const char* file, uint32_t addr);
void JLINKARM_BeginDownload(uint32_t flags);
int JLINKARM_EndDownload();
}

#endif
