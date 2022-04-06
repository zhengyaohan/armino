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
// Copyright (C) 2015-2021 Apple Inc. All Rights Reserved.

#include <arpa/inet.h>
#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "GDBServer.h"
#include "Utils.h"

#define DREGS_BASE 0x100

static TargetWord hex(const std::string& s) {
    return std::stoul(s, nullptr, 16);
}

static std::vector<TargetWord> ToBigEndian(const std::vector<TargetWord>& original) {
    std::vector<TargetWord> result = original;
    for (auto& v : result)
        v = htonl(v);
    return result;
}

static std::vector<TargetWord> FromBigEndian(const std::vector<TargetWord>& original) {
    std::vector<TargetWord> result = original;
    for (auto& v : result)
        v = ntohl(v);
    return result;
}

static std::string StringFromData(const std::vector<uint8_t>& buffer) {
    return std::string(buffer.begin(), buffer.end());
}

static std::vector<uint8_t> DataFromString(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

static std::vector<uint8_t> DecodeHex(const std::string& str) {
    std::vector<uint8_t> result;
    assert((str.size() % 2) == 0);
    for (size_t i = 0; i < str.size(); i += 2) {
        result.push_back(hex(str.substr(i, 2)));
    }
    return result;
}

static bool Match(const std::string& str, const std::string& needle, std::string& rest) {
    if (str.rfind(needle, 0) == 0) {
        rest = str.substr(needle.size());
        return true;
    }
    return false;
}

static std::vector<std::string> Split(const std::string& str, char delim) {
    std::stringstream ss(str);
    std::string word;
    std::vector<std::string> result;
    while (std::getline(ss, word, delim)) {
        result.push_back(word);
    }
    return result;
}

static std::array<TargetAddress, 2> ParseOffsetAndLength(const std::string& str) {
    auto parts = Split(str, ',');
    assert(parts.size() == 2);
    return std::array<TargetAddress, 2> { hex(parts[0]), hex(parts[1]) };
}

GDBServer::GDBServer(JLink& jlink, const std::map<std::string, std::string>& opts)
    : jlink(jlink)
    , serial(0)
    , port(8888)
    , error(false)
    , running(false) {
    if (opts.find("select") != opts.end()) {
        serial = std::stol(std::get<1>(*opts.find("select")), nullptr, 16);
    }

    if (opts.find("port") != opts.end()) {
        port = std::stoi(std::get<1>(*opts.find("port")));
    }

    threads.push_back(1);
}

void GDBServer::Warn(const std::string& msg) {
    std::cout << "GDB server: " << msg << std::endl;
}

void GDBServer::Error(const std::string& msg) {
    Warn(msg);
    error = true;
}

std::string GDBServer::ReadPacket(bool sendAck) {
    static auto BREAK = std::string("\x03");

    std::string data;
    do {
        std::vector<uint8_t> start;
        while (true) {
            bool timeout;
            std::tie(start, timeout) = Read(1);
            if (timeout) {
                assert(!start.size());
                if (running && jlink.IsHalted()) {
                    // Simulate a BREAK if the target stops
                    return BREAK;
                }
                continue;
            }
            if (!start.size()) {
                return std::string();
            }
            if (!jlink.IsHalted() && start[0] == 3) {
                // Pass the BREAK to the caller
                return BREAK;
            }
            if (start[0] == '$') {
                break;
            }
        }
        std::stringstream ss;
        while (true) {
            std::vector<uint8_t> more;
            bool timeout;
            std::tie(more, timeout) = Read(4096);
            if (timeout) {
                assert(!more.size());
                continue;
            }
            if (!more.size()) {
                return std::string();
            }
            ss.write((const char*) more.data(), more.size());
            if (ss.str().size() >= 3 && ss.str().end()[-3] == '#') {
                break;
            }
        }
        auto pkt = ss.str();
        auto csum = hex(pkt.substr(pkt.size() - 2, 2));
        data = pkt.substr(0, pkt.size() - 3);
        uint8_t sum = 0;
        for (auto d : data) {
            sum += d;
        }
        if (csum != sum) {
            Warn("receive checksum error, requesting retransmission");
            Send(std::vector<uint8_t>() = { '-' });
            continue;
        }
    } while (false);
    if (sendAck) {
        Send(std::vector<uint8_t>() = { '+' });
    }
    return data;
}

void GDBServer::SendPacket(const std::string& data) {
    std::stringstream ss;
    ss << '$';
    ss << data;
    ss << '#';
    uint8_t sum = 0;
    for (auto d : data) {
        sum += d;
    }
    ss << std::hex << std::setw(2) << std::setfill('0') << (uint32_t) sum;
    auto pkt = ss.str();
    std::cout << pkt << std::endl;
    Send(lastPacket = std::vector<uint8_t>(pkt.begin(), pkt.end()));
}

std::string GDBServer::Lookup(const std::string& name) {
    assert(name == "target.xml");
    std::stringstream ss;
    ss << "<!DOCTYPE feature SYSTEM \"gdb-target.dtd\">" << std::endl;
    ss << "<target version=\"1.0\">" << std::endl;
    ss << "  <architecture>arm</architecture>" << std::endl;
    std::string group;
    auto quote = [](const std::string& str) -> std::string { return "\"" + str + "\""; };
    auto reg = [this, &ss, &group, quote](const std::string& name, auto type) {
        auto bitsize = "32";
        if (type == "ieee_double") {
            bitsize = "64";
        }
        size_t handle;
        if (name[0] == 'd') {
            handle = DREGS_BASE + std::stoul(name.substr(1));
        } else {
            handle = jlink.GetRegHandle(name);
        }
        ss << "    <reg name=" << quote(name) << " bitsize=" << quote(bitsize)
           << " regnum=" << quote(std::to_string(handle)) << " type=" << quote(type) << " group=" << quote(group)
           << "/>" << std::endl;
    };
    auto feature = [&ss, &group, quote](auto profile, auto profile_group, auto content) {
        ss << "  <feature name=" << quote(profile) << ">" << std::endl;
        group = profile_group;
        content();
        ss << "  </feature>" << std::endl;
    };
    feature("org.gnu.gdb.arm.m-profile", "general", [&reg] {
        for (size_t r = 0; r < 13; ++r) {
            reg("r" + std::to_string(r), "uint32");
        }
        reg("sp", "data_ptr");
        reg("lr", "uint32");
        reg("pc", "code_ptr");
        reg("xpsr", "uint32");
    });
    feature("org.gnu.gdb.arm.m-system", "general", [&reg] {
        reg("msp", "uint32");
        reg("psp", "uint32");
        reg("primask", "uint32");
        reg("basepri", "uint32");
        reg("faultmask", "uint32");
        reg("control", "uint32");
    });
    feature("org.gnu.gdb.arm.m-float", "float", [&reg] {
        reg("fpscr", "uint32");
        for (size_t i = 0; i < 32; ++i) {
            reg("s" + std::to_string(i), "float");
        }
        for (size_t i = 0; i < 16; ++i) {
            reg("d" + std::to_string(i), "ieee_double");
        }
    });
    ss << "</target>" << std::endl;
    return ss.str();
}

void GDBServer::Transfer(const std::string& name, size_t offset, size_t length) {
    auto data = Lookup(name);
    auto chunk = data.substr(offset, length);
    if (chunk.size() == 0) {
        SendPacket("l");
        return;
    }
    bool more = (offset + length) < data.size();
    SendPacket((more ? "m" : "l") + chunk);
}

std::string GDBServer::ReadRegs(const std::vector<uint32_t>& regs) {
    return EncodeHex(ToBigEndian(jlink.ReadRegs(regs)));
}

std::string GDBServer::ReadAllRegs() {
    static std::vector<uint32_t> regs;

    if (!regs.size()) {
        for (size_t r = 0; r < 13; ++r) {
            regs.push_back(jlink.GetRegHandle("r" + std::to_string(r)));
        }
        regs.push_back(jlink.GetRegHandle("sp"));
        regs.push_back(jlink.GetRegHandle("lr"));
        regs.push_back(jlink.GetRegHandle("pc"));
    }
    return ReadRegs(regs);
}

bool GDBServer::IsDoubleReg(uint32_t handle, std::vector<uint32_t>& regs) {
    if (handle < DREGS_BASE) {
        return false;
    }
    handle -= DREGS_BASE;
    handle *= 2;
    handle += jlink.GetRegHandle("s0");
    regs = std::vector<uint32_t> { handle + 1, handle };
    return true;
}

std::string GDBServer::ReadReg(uint32_t handle) {
    std::vector<uint32_t> regs;
    if (IsDoubleReg(handle, regs)) {
        return ReadRegs(regs);
    }
    return ReadRegs(std::vector<uint32_t> { handle });
}

void GDBServer::WriteReg(uint32_t handle, const std::string& value) {
    std::vector<uint32_t> regs;
    if (IsDoubleReg(handle, regs)) {
        assert(value.size() == 16);
        WriteReg(regs[0], value.substr(0, 8)); // swapped word order
        WriteReg(regs[1], value.substr(8));
        return;
    }
    assert(value.size() == 8);
    jlink.WriteRegs(std::vector<uint32_t> { handle }, FromBigEndian(std::vector<uint32_t> { hex(value) }));
}

void GDBServer::Resume(bool step) {
    assert(!running);
    if (step) {
        jlink.Step();
    } else {
        jlink.Resume();
    }
    running = true;
}

std::string GDBServer::ModifyBP(TargetAddress addr, uint32_t length, bool hw, bool remove) {
    if (remove) {
        jlink.ClearBP(addr);
    } else {
        jlink.SetBP(addr, hw);
    }
    return "OK";
}

std::string GDBServer::ModifyWP(TargetAddress addr, uint32_t length, bool read, bool write, bool remove) {
    if (remove) {
        jlink.ClearWP(addr);
    } else {
        if (length != 1 && length != 2 && length != 4) {
            length = 4;
        }
        if (!jlink.SetWP(addr, length, read, write)) {
            return "E01";
        }
    }
    return "OK";
}

std::string GDBServer::MonitorCommand(const std::string& cmd) {
    std::string result;
    std::string rest;
    if (Match(cmd, "r", rest)) {
        jlink.Reset();
        return "OK";
    }
    if (result == "") {
        result = "List of monitor commands:\n\n"
                 "reset -- reset the CPU and halt\n"
                 "Command name abbreviations are allowed if unambiguous.\n";
    }
    return EncodeHex(DataFromString(result));
}

static void log(const char* str) {
    std::cout << str << std::endl;
}

void GDBServer::run() {
    Listen(port);
    for (;;) {
        std::cout << "Listening on port " << std::dec << port << std::endl;
        Accept();
        jlink.Open(serial);
        jlink.Test();
        jlink.Halt();
        std::string supported;
        bool sendAck = true;
        std::map<uint8_t, int> selectedThread;
        while (!error) {
            auto pkt = ReadPacket(sendAck);
            if (!pkt.size()) {
                break;
            }
            std::cout << "CMD:" << pkt << std::endl;
            std::string rest;
            switch (pkt[0]) {
                case '+':
                    lastPacket.clear();
                    continue;
                case '-':
                    Warn("receiver checksum error, retransmitting");
                    assert(lastPacket.size() > 0);
                    Send(lastPacket);
                    continue;
                case 0x03: { // interrupt
                    assert(running);
                    if (!jlink.IsHalted()) {
                        jlink.Halt();
                        assert(jlink.IsHalted());
                    }
                    std::cout << "stopped at " << std::hex
                              << jlink.ReadRegs(std::vector<TargetWord> { jlink.GetRegHandle("pc") })[0] << std::endl;
                    running = false;
                    // fall through
                }
                case '?': {
                    std::string info;
                    if (supported.find("hwbreak+")) {
                        auto pc = jlink.ReadRegs(std::vector<TargetWord> { jlink.GetRegHandle("pc") })[0];
                        auto reasons = jlink.GetHaltReasons();
                        std::string reason = "";
                        for (const auto& r : reasons) {
                            if (r.HaltReason == (uint32_t) JLinkHaltReasonType::CODE_BREAKPOINT) {
                                for (const auto& bp : jlink.Breakpoints()) {
                                    if (bp.Addr == pc) {
                                        reason = "hwbreak:;";
                                        break;
                                    }
                                }
                            }
                            if (r.HaltReason == (uint32_t) JLinkHaltReasonType::DATA_BREAKPOINT) {
                                reason = "hwbreak:;";
                            }
                        }
                        info += reason;
                    }
                    info += "thread:" + EncodeHex(std::vector<uint32_t>(threads.begin(), threads.begin() + 1)) + ";";
                    SendPacket("T05" + info);
                    continue;
                }
                case 'c':
                    assert(pkt.size() == 1);
                    Resume(false);
                    continue;
                case 'g':
                    SendPacket(ReadAllRegs());
                    continue;
                case 'H': {
                    assert(pkt.size() > 2);
                    assert(pkt.substr(2) == std::to_string(std::stol(pkt.substr(2))));
                    int tid = std::stol(pkt.substr(2));
                    assert(tid >= -1);
                    assert(tid <= 0 || std::find(threads.begin(), threads.end(), tid) != threads.end());
                    selectedThread.insert(std::make_pair(pkt[1], std::stol(pkt.substr(2))));
                    SendPacket("OK");
                    continue;
                }
                case 'm': {
                    auto addressAndBytes = ParseOffsetAndLength(pkt.substr(1));
                    int status;
                    std::vector<uint8_t> bytes;
                    std::tie(status, bytes) = jlink.ReadMemory(addressAndBytes[0], addressAndBytes[1]);
                    CHECKED(status);
                    SendPacket(EncodeHex(bytes));
                    continue;
                }
                case 'M': {
                    auto parts = Split(pkt.substr(1), ':');
                    auto addressAndBytes = ParseOffsetAndLength(parts[0]);
                    assert(parts[1].size() == addressAndBytes[1] * 2);
                    jlink.WriteMemory(addressAndBytes[0], DecodeHex(parts[1]));
                    SendPacket("OK");
                    continue;
                }
                case 'p':
                    SendPacket(ReadReg(hex(pkt.substr(1))));
                    continue;
                case 'P': {
                    auto handleAndValue = Split(pkt.substr(1), '=');
                    WriteReg(hex(handleAndValue[0]), handleAndValue[1]);
                    SendPacket("OK");
                    continue;
                }
                case 'q':
                    if (Match(pkt, "qSupported", rest)) {
                        SendPacket(
                                "PacketSize=65536;qXfer:memory-map:read-;QStartNoAckMode+;hwbreak+;qXfer:features:"
                                "read+");
                        continue;
                    }
                    if (Match(pkt, "qXfer:features:read:", rest)) {
                        auto fileAndChunk = Split(rest, ':');
                        assert(fileAndChunk.size() == 2);
                        auto file = fileAndChunk[0];
                        auto startAndLength = ParseOffsetAndLength(fileAndChunk[1]);
                        Transfer(file, startAndLength[0], startAndLength[1]);
                        continue;
                    }
                    if (Match(pkt, "qRcmd,", rest)) {
                        auto cmd = StringFromData(DecodeHex(rest));
                        SendPacket(MonitorCommand(cmd));
                        continue;
                    }
                    if (Match(pkt, "qfThreadInfo", rest)) {
                        SendPacket("m" + EncodeHex(threads, ","));
                        continue;
                    }
                    if (Match(pkt, "qsThreadInfo", rest)) {
                        SendPacket("l");
                        continue;
                    }
                    if (Match(pkt, "qSymbol::", rest)) {
                        SendPacket("OK");
                        continue;
                    }
                    break;
                case 'Q':
                    if (Match(pkt, "QStartNoAckMode", rest)) {
                        sendAck = false;
                        SendPacket("OK");
                        continue;
                    }
                    break;
                case 's':
                    assert(pkt.size() == 1);
                    Resume(true);
                    continue;
                case 'v':
                    if (Match(pkt, "vKill;", rest)) {
                        jlink.ExecuteCommand("ClrAllBPs");
                        SendPacket("OK");
                        continue;
                    }
                    break;
                case 'z':
                case 'Z': {
                    if (pkt[1] < '0' || pkt[1] > '4') {
                        break;
                    }
                    auto partsAndCond = Split(pkt, ';');
                    assert(partsAndCond.size() >= 1);
                    std::cout << partsAndCond[0] << std::endl;
                    auto parts = Split(partsAndCond[0], ',');
                    std::cout << parts.size() << std::endl;
                    assert(parts.size() == 3);
                    auto remove = parts[0][0] == 'z';
                    auto type = parts[0][1];
                    auto address = hex(parts[1]);
                    auto length = hex(parts[2]);
                    if (type == '0' || type == '1') {
                        SendPacket(ModifyBP(address, length, type == '1', remove));
                        continue;
                    }
                    SendPacket(ModifyWP(
                            address, length, (type == '3' || type == '4'), (type == '2') || (type == '4'), remove));
                    continue;
                }
                default:
                    break;
            }
            SendPacket("");
        }
        if (jlink.IsHalted()) {
            jlink.Resume();
        }
        jlink.Close();
        std::cout << "Disconnected." << std::endl;
        break;
    }
}
