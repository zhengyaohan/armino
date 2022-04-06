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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <sstream>

#include "JLink.h"

using namespace std;

JLink::JLink()
    : logHandler(nullptr) {
}

void JLink::SetDebugOutHandler(void (*handler)(const char*)) {
    logHandler = handler;
}

void JLink::Log(const char* str) {
    if (logHandler) {
        logHandler(str);
        return;
    }
}

std::string JLink::DLLVersion() {
    std::stringstream ss;
    int version = JLINKARM_GetDLLVersion();
    int major = version / 10000;
    int minor = (version / 100) % 100;
    int rev = (version % 100);
    ss << major << '.' << minor << '.' << rev;
    return ss.str();
}

std::vector<JLinkConnectInfo> JLink::Emulators(JLinkHost host) {
    static std::vector<JLinkConnectInfo> result;
    if (!result.size()) {
        size_t n = JLINKARM_EMU_GetList(host, 0, 0);
        result.resize(n);
        JLINKARM_EMU_GetList(host, result.data(), n);
    }
    return result;
}

std::vector<JLinkDeviceInfo> JLink::SupportedDevices() {
    static std::vector<JLinkDeviceInfo> result;
    if (!result.size()) {
        size_t n = JLINKARM_DEVICE_GetInfo(-1, 0);
        result.resize(n);
        for (size_t i = 0; i < n; ++i) {
            result[i].SizeOfStruct = sizeof(result[i]);
            JLINKARM_DEVICE_GetInfo(i, &result[i]);
        }
    }
    return result;
}

int JLink::ExecuteCommand(const std::string& cmd) {
    char msg[512];
    int err = JLINKARM_ExecCommand(cmd.c_str(), msg, sizeof(msg));
    if (*msg) {
        Log(msg);
    }
    return err;
}

bool JLink::Open(uint32_t serial) {
    if (serial) {
        int err = JLINKARM_EMU_SelectByUSBSN(serial);
        if (err < 0) {
            std::cerr << "Error: No JLINK device with serial number or USB index " << std::hex << serial << " found"
                      << std::endl;
            std::cerr << "Please run \"jlink --list\" to see the list of attached devices" << std::endl;
            return false;
        }
    }
    const char* msg = JLINKARM_OpenEx(logHandler, logHandler);
    if (msg) {
        std::cerr << msg << std::endl;
        return false;
    }
    CHECKED(JLINKARM_UpdateFirmwareIfNewer());
    CHECKED(ExecuteCommand("EnableRemarks = 1"));
    CHECKED(ExecuteCommand("Device = nRF52840_xxAA"));
    CHECKED(JLINKARM_CORESIGHT_Configure(""));
    int mask;
    JLINKARM_TIF_GetAvailable(&mask);
    assert(mask & (1 << (uint32_t) JLinkInterface::SWD));
    CHECKED(JLINKARM_TIF_Select(JLinkInterface::SWD));
    return true;
}

void JLink::Close() {
    if (JLINKARM_IsOpen()) {
        CHECKED(ExecuteCommand("ClrAllBPs"));
        JLINKARM_Close();
    }
}

int JLink::Reset() {
    JLINKARM_SetInitRegsOnReset(1);
    return JLINKARM_Reset();
}

std::vector<JLinkBreakpointInfo> JLink::Breakpoints() {
    std::vector<JLinkBreakpointInfo> result;
    size_t n = JLINKARM_GetNumBPs();
    result.resize(n);
    for (size_t i = 0; i < n; ++i) {
        result[i].SizeOfStruct = sizeof result[i];
        result[i].Handle = 0; // we'll use the index
        int n = JLINKARM_GetBPInfoEx(i, &result[i]);
        assert(n >= 0);
    }
    return result;
}

bool JLink::SetBP(TargetAddress addr, bool hw) {
    uint32_t flags = (uint32_t) JLinkBreakpoint::THUMB;
    if (!hw) {
        JLINKARM_EnableSoftBPs(1);
    } else {
        flags |= (uint32_t) JLinkBreakpoint::HW;
    }
    return JLINKARM_SetBPEx(addr, flags) > 0;
}

size_t JLink::ClearBP(TargetAddress addr) {
    size_t n = 0;
    std::vector<uint32_t> handles;
    for (const auto& bp : Breakpoints()) {
        if (bp.Addr == addr) {
            handles.push_back(bp.Handle);
            n += bp.UseCnt;
        }
    }
    for (const auto h : handles) {
        JLINKARM_ClrBPEx(h);
    }
    return n;
}

std::vector<JLinkWatchpointInfo> JLink::Watchpoints() {
    std::vector<JLinkWatchpointInfo> result;
    size_t n = JLINKARM_GetNumWPs();
    result.resize(n);
    for (size_t i = 0; i < n; ++i) {
        result[i].SizeOfStruct = sizeof result[i];
        result[i].Handle = 0; // we'll use the index
        int n = JLINKARM_GetWPInfoEx(i, &result[i]);
        assert(n >= 0);
    }
    return result;
}

bool JLink::SetWP(TargetAddress addr, size_t size, bool read, bool write) {
    assert(size == 1 || size == 2 || size == 4);
    assert(read || write);
    JLinkDataEvent ev;
    ev.SizeOfStruct = sizeof ev;
    ev.Addr = addr;
    ev.AddrMask = 0;
    if (size >= 2) {
        ev.AddrMask |= 1;
    }
    if (size >= 4) {
        ev.AddrMask |= 2;
    }
    ev.Data = 0;
    ev.DataMask = 0;
    ev.Access = 0;
    ev.AccessMask = ((uint32_t) JLinkAccessMaskFlags::SIZE) | ((uint32_t) JLinkAccessMaskFlags::PRIV);
    if (read & write) {
        ev.AccessMask |= ((uint32_t) JLinkAccessMaskFlags::DIR);
    } else {
        if (read) {
            ev.Access |= ((uint32_t) JLinkAccessFlags::READ);
        }
        if (write) {
            ev.Access |= ((uint32_t) JLinkAccessFlags::WRITE);
        }
    }
    uint32_t handle;
    return JLINKARM_SetDataEvent(&ev, &handle) > 0;
}

size_t JLink::ClearWP(TargetAddress addr) {
    size_t n = 0;
    std::vector<uint32_t> handles;
    for (const auto& bp : Watchpoints()) {
        if (bp.Addr == addr) {
            handles.push_back(bp.Handle);
            n++;
        }
    }
    for (const auto h : handles) {
        JLINKARM_ClrDataEvent(h);
    }
    return n;
}

std::vector<JLinkMOEInfo> JLink::GetHaltReasons() {
    static const size_t MAX = 8;
    std::vector<JLinkMOEInfo> reasons;
    reasons.resize(MAX);
    int n = JLINKARM_GetMOEs(reasons.data(), MAX);
    assert(n >= 0);
    reasons.resize(n);
    return reasons;
}

void JLink::Step(bool thumb) {
    if (thumb) {
        CHECKED(JLINKARM_StepComposite());
        return;
    }
    CHECKED(JLINKARM_Step());
}

void JLink::Resume() {
    JLINKARM_GoEx(0, (uint32_t) JLinkFlags::GO_OVERSTEP_BP);
}

std::vector<uint32_t> JLink::RegisterHandles() {
    static std::vector<uint32_t> result;
    if (!result.size()) {
        result.resize(256);
        size_t n = JLINKARM_GetRegisterList(result.data(), result.size());
        result.resize(n);
    }
    return result;
}

std::vector<std::string> JLink::RegisterNames() {
    static std::vector<std::string> result;
    if (!result.size()) {
        const auto& regs = RegisterHandles();
        for (const auto& r : regs) {
            auto name = std::string(JLINKARM_GetRegisterName(r));
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            auto lparen = name.find('(');
            auto rparen = name.find(')');
            if (lparen != std::string::npos && rparen != std::string::npos) {
                name = name.substr(lparen + 1, rparen - lparen - 1);
            }
            if (name == "r14") {
                name = "lr";
            }
            if (name.find("fps") == 0 && name != "fpscr") {
                name = name.substr(2);
            }
            result.push_back(name);
        }
    }
    return result;
}

uint32_t JLink::GetRegHandle(const std::string& name) {
    static std::map<const std::string, uint32_t> map;
    if (!map.size()) {
        const auto& names = RegisterNames();
        const auto& handles = RegisterHandles();
        assert(names.size() == handles.size());
        for (size_t i = 0; i < names.size(); ++i) {
            map.insert(std::make_pair(names[i], handles[i]));
        }
    }
    auto it = map.find(name);
    assert(it != map.end());
    return it->second;
}

std::vector<TargetWord> JLink::ReadRegs(const std::vector<uint32_t>& regs) {
    std::vector<TargetWord> result;
    std::vector<uint8_t> status; // unreliable, ignore
    result.resize(regs.size());
    status.resize(regs.size());
    CHECKED(JLINKARM_ReadRegs(regs.data(), result.data(), status.data(), regs.size()));
    return result;
}

void JLink::WriteRegs(const std::vector<uint32_t>& regs, const std::vector<TargetWord>& values) {
    assert(regs.size() == values.size());
    std::vector<uint8_t> statuses;
    statuses.resize(regs.size());
    CHECKED(JLINKARM_WriteRegs(regs.data(), values.data(), statuses.data(), regs.size()));
}

TargetWord JLink::ReadReg(const std::string& name) {
    std::vector<uint32_t> regs = { GetRegHandle(name) };
    auto values = ReadRegs(regs);
    assert(values.size() == 1);
    return values[0];
}

std::tuple<int, std::vector<uint8_t>> JLink::ReadMemory(TargetAddress addr, uint32_t bytes) {
    std::vector<uint8_t> result;
    result.resize(bytes);
    int n = JLINKARM_ReadMemEx(addr, bytes, result.data(), 0);
    int status = (n == result.size() ? 0 : -1);
    return std::make_tuple(status, result);
}

void JLink::WriteMemory(TargetAddress addr, const std::vector<uint8_t>& bytes) {
    int n = JLINKARM_WriteMemEx(addr, bytes.size(), bytes.data(), 0);
    if (n == bytes.size()) {
        n = 0;
    }
    CHECKED(n);
}

void JLink::Flash(
        const std::vector<std::pair<TargetAddress, std::vector<uint8_t>>>& code,
        JLinkProgressCallback callback) {
    if (callback) {
        JLINK_SetFlashProgProgressCallback(callback);
    }
    JLINKARM_BeginDownload(0);
    for (const auto& segment : code) {
        WriteMemory(segment.first, segment.second);
    }
    JLINKARM_EndDownload();
    if (callback) {
        JLINK_SetFlashProgProgressCallback(nullptr);
    }
}

struct RTTBuffer {
    TargetAddress Name;
    TargetAddress Address;
    uint32_t Size;
    uint32_t WriteOffset;
    uint32_t ReadOffset;
    uint32_t Flags;
};

struct RTTControlBlock {
    char Magic[16];
    uint32_t UpBufferCount;
    uint32_t DownBufferCount;
};

template <typename T> static T Read(JLink& jlink, TargetAddress address) {
    int status;
    std::vector<uint8_t> bytes;
    std::tie(status, bytes) = jlink.ReadMemory(address, sizeof(T));
    CHECKED(status);
    T value;
    memcpy(&value, bytes.data(), sizeof(T));
    return value;
}

template <typename T> static void Write(JLink& jlink, TargetAddress address, const T& value) {
    std::vector<uint8_t> bytes;
    bytes.resize(sizeof(T));
    ;
    memcpy(bytes.data(), &value, sizeof(T));
    jlink.WriteMemory(address, bytes);
}

static TargetAddress RTTControlBlockAddress = 0;
static std::vector<uint32_t> OriginalRTTFlags;

static TargetAddress RTTBufferAddress(size_t n) {
    return RTTControlBlockAddress + sizeof(RTTControlBlock) + n * sizeof(RTTBuffer);
}

void JLink::RTTStart(TargetAddress controlBlockAddress, uint32_t mode) {
    RTTControlBlockAddress = controlBlockAddress;
    auto cb = Read<RTTControlBlock>(*this, RTTControlBlockAddress);
    assert(!memcmp(cb.Magic, "SEGGER RTT", 10));
    OriginalRTTFlags.resize(cb.UpBufferCount);
    for (size_t n = 0; n < cb.UpBufferCount; ++n) {
        TargetAddress flagsAddr = RTTBufferAddress(n) + offsetof(RTTBuffer, Flags);
        auto flags = Read<uint32_t>(*this, flagsAddr);
        OriginalRTTFlags[n] = flags;
        if ((flags & 3) != 2) {
            std::cout << "Switching RTT up channel " << n << " to blocking mode ..." << std::endl;
            flags &= (~3);
            flags |= mode;
            Write(*this, flagsAddr, flags);
        }
    }
}

void JLink::RTTStop() {
    if (!RTTControlBlockAddress) {
        return;
    }
    for (size_t n = 0; n < OriginalRTTFlags.size(); ++n) {
        auto flags = OriginalRTTFlags[n];
        TargetAddress flagsAddr = RTTBufferAddress(n) + offsetof(RTTBuffer, Flags);
        Write(*this, flagsAddr, flags);
    }
}

std::string JLink::RTTRead(size_t n) {
    auto buffer = Read<RTTBuffer>(*this, RTTBufferAddress(n));
    if (buffer.ReadOffset == buffer.WriteOffset) {
        return "";
    }
    std::stringstream ss;
    auto fetch = [this, &buffer, &ss](uint32_t StopOffset) {
        int status;
        std::vector<uint8_t> mem;
        std::tie(status, mem) = ReadMemory(buffer.Address + buffer.ReadOffset, StopOffset - buffer.ReadOffset);
        CHECKED(status);
        buffer.ReadOffset = StopOffset;
        if (buffer.ReadOffset >= buffer.Size) {
            buffer.ReadOffset = 0;
        }
        ss.write((const char*) mem.data(), mem.size());
    };
    if (buffer.ReadOffset > buffer.WriteOffset) {
        fetch(buffer.Size);
    }
    if (buffer.ReadOffset < buffer.WriteOffset) {
        fetch(buffer.WriteOffset);
    }
    // Only the host writes to ReadOffset, so there is never a race condition
    Write(*this, RTTBufferAddress(n) + offsetof(RTTBuffer, ReadOffset), buffer.ReadOffset);
    return ss.str();
}
