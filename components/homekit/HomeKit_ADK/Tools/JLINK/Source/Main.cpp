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

#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "Arguments.h"
#include "GDBServer.h"
#include "JLink.h"
#include "Loader.h"
#include "Utils.h"

static std::string slurp(const std::string& path) {
    std::ostringstream buf;
    std::ifstream input(path.c_str());
    buf << input.rdbuf();
    return buf.str();
}

static void log(const char* str) {
    std::cout << str << std::endl;
}

static void NoOutput(JLink& jlink, size_t n) {
}

static void Output(JLink& jlink, size_t n) {
    std::cout << jlink.RTTRead(n);
}

static void ProgressCallback(const char* action, const char* msg, int percentage) {
    if (msg) {
        log(msg);
    }
}

static bool exist(const std::string& path) {
    std::ifstream input(path.c_str());
    return input.good();
}

template <typename T>
static int WithDevice(JLink& jlink, const std::map<std::string, std::string>& optvals, T closure) {
    uint32_t serial = 0;

    if (optvals.find("select") != optvals.end()) {
        serial = std::stol(std::get<1>(*optvals.find("select")), nullptr, 16);
    }

    bool resume = false;
    if (optvals.find("resume") != optvals.end()) {
        resume = true;
    }

    std::cout << "Connecting to target #" << std::hex << serial << std::dec << " ..." << std::endl;

    if (!jlink.Open(serial)) {
        return -1;
    }
    jlink.Halt();
    assert(jlink.IsHalted());

    int status = closure();

    jlink.Halt();
    assert(jlink.IsHalted());
    jlink.RTTStop();
    if (resume) {
        jlink.Resume();
    }
    jlink.Close();

    return status;
}

template <typename T>
static int WithDeviceRetryIfFail(
        JLink& jlink,
        const std::map<std::string, std::string>& optvals,
        T closure,
        int numTries = 6) {
    uint32_t serial = 0;

    if (optvals.find("select") != optvals.end()) {
        serial = std::stol(std::get<1>(*optvals.find("select")), nullptr, 16);
    }

    bool resume = false;
    if (optvals.find("resume") != optvals.end()) {
        resume = true;
    }

    std::cout << "Connecting to target #" << std::hex << serial << std::dec << " ..." << std::endl;

    // If JLink wasn't closed properly, JLINKARM_OpenEx() may fail.
    // Moreover, if it fails with getting garbage over JLink the process may never be able to open JLink again.
    // Hence, try to connect it through a subprocess and retry multiple times.
    int childstatus;
    for (int i = 0; i < numTries; i++) {
        pid_t pid = fork();
        if (pid) {
            // parent
            pid_t terminated;
            do {
                terminated = wait(&childstatus);
            } while (terminated != pid);
            if (childstatus == 0) {
                break;
            } else if (numTries > 1) {
                std::cerr << "[WARN] retry #" << (i + 1) << std::endl;
            }
        } else {
            // child
            if (!jlink.Open(serial)) {
                sleep(1);
                exit(1);
            }
            jlink.Halt();
            assert(jlink.IsHalted());

            int status = closure();

            jlink.Halt();
            assert(jlink.IsHalted());
            jlink.RTTStop();
            if (status == 0 && resume) {
                jlink.Resume();
            }
            jlink.Close();
            if (status != 0) {
                sleep(1);
            }
            exit(status);
        }
    }

    return childstatus;
}

static int FlashAndRun(
        JLink& jlink,
        const std::vector<std::string>& optfiles,
        const std::map<std::string, std::string>& optvals) {
    bool realtime = false;
    if (optvals.find("realtime") != optvals.end()) {
        realtime = true;
    }

    std::vector<std::pair<TargetAddress, std::vector<uint8_t>>> code;
    std::map<std::string, TargetAddress> syms;
    for (const auto& path : optfiles) {
        std::cout << "Parsing: " << path << std::endl;
        if (!exist(path)) {
            std::cout << "Error: file " << path << " not found" << std::endl;
            return -1;
        }
        auto file = slurp(path);
        if (file.size() >= 4 && file[0] == ELFMAG0 && file[1] == ELFMAG1 && file[2] == ELFMAG2 && file[3] == ELFMAG3) {
            LoadELF(file, code, syms);
        } else {
            LoadIHEX(file, code);
        }
    }

    // make sure the segments we collected don't overlap
    auto overlaps = [](const auto& a, const auto& b) {
        return (a.first < (b.first + b.second.size())) && (b.first < (a.first + a.second.size()));
    };
    for (size_t i = 0; i < code.size(); ++i) {
        for (size_t j = 0; j < code.size(); ++j) {
            if (i == j) {
                continue;
            }
            assert(!overlaps(code[i], code[j]));
        }
    }

    std::cout << "Update flash ..." << std::endl;

    jlink.Flash(code, ProgressCallback);
    jlink.Reset();

    std::cout << "Code is up to date" << std::endl;

    if (optvals.find("run") == optvals.end()) {
        return 0;
    }

    auto label = [&syms](const std::string& name) -> TargetAddress {
        std::cout << "Looking up symbol: \"" << name << "\"" << std::endl;
        if (syms.find(name) == syms.end()) {
            std::cout << "Warning symbol \"" << name << "\" not found" << std::endl;
            return 0;
        }
        return (syms[name] & ~1); // strip off THUMB bit
    };

    assert(jlink.IsHalted());
    TargetAddress _exit = label("_exit");
    assert(_exit);
    TargetAddress _SEGGER_RTT = label("_SEGGER_RTT");
    TargetAddress _SEGGER_post_INIT = label("SEGGER_RTT_WriteNoLock");
    jlink.SetBP(_exit, true);
    if (_SEGGER_post_INIT != 0) {
        jlink.SetBP(_SEGGER_post_INIT, true);
    }

    std::cout << "Running target ..." << std::endl;

    jlink.Resume();

    void (*rtt)(JLink & jlink, size_t n) = NoOutput;
    int status;

    while (true) {
        while (!jlink.IsHalted()) {
            rtt(jlink, 0);
        }
        // After stopping flush the RTT ring buffer one more time
        rtt(jlink, 0);
        TargetAddress pc = jlink.ReadRegs(std::vector<uint32_t> { jlink.GetRegHandle("pc") })[0];
        // This breakpoint is hit after the first output ot the RTT
        if (_SEGGER_post_INIT != 0 && _SEGGER_RTT != 0 && pc == _SEGGER_post_INIT) {
            jlink.RTTStart(_SEGGER_RTT, realtime ? 0 : 2);
            jlink.ClearBP(_SEGGER_post_INIT);
            rtt = Output;
            jlink.Resume();
            continue;
        }
        if (pc == _exit) {
            status = (int) jlink.ReadRegs(std::vector<uint32_t> { jlink.GetRegHandle("r0") })[0];
            std::cout << "Stopped at _exit" << std::endl;
            std::cout << "Status: " << std::dec << status << std::endl;
            if (optvals.find("gdbonabort") != optvals.end()) {
                GDBServer server(jlink, optvals);
                server.run();
            }
            return status;
        }
        std::cout << "Abort at " << std::hex << "0x" << pc << std::dec << std::endl;
        if (optvals.find("gdbonabort") != optvals.end()) {
            GDBServer server(jlink, optvals);
            server.run();
            return 0;
        }
        return -1;
    }
}

int main(int argc, const char* argv[]) {
    std::vector<std::tuple<std::string, std::string>> opts = {
        { "=", "Actions" },
        { "list", "list attached devices" },
        { "flash", "flash an application (you can provide multiple non-overlapping IHEX or ELF binaries)" },
        { "run", "flash and run an application (accepts file arguments like --flash)" },
        { "reset", "reset the attached device" },
        { "erase", "erase the attached device" },
        { "halt", "halt the cpu core of the attached device" },
        { "memrd address",
          "read n bytes from the provided address and return space separated 4 byte words in big endian format" },
        { "n bytes", "Used in conjuction with memrd to allow specifying number of bytes to read." },
        { "gdbserver", "launch as GDB server and wait for incoming connections" },
        { "gdbonabort", "launch GDB server when the program aborts or exits" },
        { "help", "print this help text" },
        { "version", "print program build date" },
        { "=", "General options" },
        { "select INDEX", "select device to use (by index)" },
        { "log", "print detailed log information while running" },
        { "=", "Options when running an application" },
        { "realtime", "don't pause target when terminal output is overflowing" },
        { "resume", "resume execution when we exit (instead of leaving the target halted)" },
        { "=", "Options for GDB server" },
        { "port PORT", "Listen on PORT (default: 8888)" },
        { "=", "Examples" },
        { "*", "jlink --list" },
        { "*", "jlink --select 1 --log --flash s140_nrf52_6.1.1_softdevice.hex HAPBase+IntTests.Oberon" },
        { "*", "jlink --port 8080 --realtime --gdbserver" },
    };
    std::map<std::string, std::string> optvals;
    std::vector<std::string> optfiles;
    std::tie(optvals, optfiles) = ParseArguments(argc, argv, opts);

    JLink jlink;

    if (optvals.find("version") != optvals.end()) {
        std::cout << "jlink-run " << __DATE__ << ' ' << __TIME__ << ' ' << "jlinkdll: " << jlink.DLLVersion()
                  << std::endl;
        return 0;
    }

    if (optvals.find("log") != optvals.end()) {
        jlink.SetLogHandler(log);
        jlink.SetDetailedLogHandler(log);
        jlink.SetErrorOutHandler(log);
        jlink.SetWarnOutHandler(log);
    }

    if (optvals.find("list") != optvals.end()) {
        auto emulators = jlink.Emulators();
        std::cout << "Index   Serial   Product" << std::endl;
        for (size_t n = 0; n < emulators.size(); ++n) {
            auto emu = emulators[n];
            std::cout << std::dec << std::setw(8) << std::setfill(' ') << std::left << n;
            std::cout << std::hex << std::setw(8) << std::setfill('0') << std::uppercase << emu.SerialNumber << ' ';
            std::cout << emu.acProduct << std::endl;
        }
        return 0;
    }

    if (optvals.find("flash") != optvals.end() || optvals.find("run") != optvals.end()) {
        return WithDevice(
                jlink, optvals, [&jlink, &optfiles, &optvals]() { return FlashAndRun(jlink, optfiles, optvals); });
    }

    if (optvals.find("gdbserver") != optvals.end()) {
        GDBServer server(jlink, optvals);
        server.run();
        return 0;
    }

    if (optvals.find("reset") != optvals.end()) {
        return WithDeviceRetryIfFail(jlink, optvals, [&jlink, &optfiles, &optvals]() {
            std::cout << "Applying system reset" << std::endl;
            return jlink.Reset();
        });
    }

    if (optvals.find("erase") != optvals.end()) {
        WithDeviceRetryIfFail(jlink, optvals, [&jlink, &optfiles, &optvals]() {
            std::cout << "Erasing user available code and UICR flash areas" << std::endl;
            return jlink.Erase();
        });

        return WithDevice(jlink, optvals, [&jlink, &optfiles, &optvals]() {
            std::cout << "Applying system reset" << std::endl;
            jlink.Reset();
            return 0;
        });
    }

    if (optvals.find("halt") != optvals.end()) {
        return WithDevice(jlink, optvals, [&jlink, &optfiles, &optvals]() {
            std::cout << "Halting cpu core" << std::endl;
            jlink.Halt();

            // Closing jlink somehow resumes the app again so waiting here indefinitely
            while (true) {
            }
            return 0;
        });
    }

    if (optvals.find("memrd") != optvals.end() && optvals.find("n") != optvals.end()) {
        TargetAddress address = std::stol(std::get<1>(*optvals.find("memrd")), nullptr, 16);
        int numBytes = std::stol(std::get<1>(*optvals.find("n")), nullptr, 10);
        if ((numBytes % 4) != 0) {
            std::cout << "Argument provided has a wrong value. Memory access not aligned, value outside of range."
                      << std::endl;
            return 1;
        }

        return WithDeviceRetryIfFail(jlink, optvals, [&jlink, &optfiles, &optvals, address, numBytes]() {
            int status;
            std::vector<uint8_t> memory;
            std::tie(status, memory) = jlink.ReadMemory(address, numBytes);
            if (status != 0) {
                std::cerr << "Failed to read from 0x" << std::hex << address << std::dec << std::endl;
                return status;
            }
            std::cout << "0x" << std::hex << address << std::dec << ": ";
            uint32_t n = 0;
            std::vector<TargetWord> result;

            // Print every 4 bytes as hex string
            for (int i = 0; i < (numBytes / 4); i++) {
                auto first = memory.cbegin() + n;
                auto last = memory.cbegin() + n + 4;
                std::vector<uint8_t> vec(first, last);

                std::string data = EncodeHex(vec);
                transform(data.begin(), data.end(), data.begin(), ::toupper);
                std::cout << data << " ";
                n = n + 4;
            }
            std::cout << std::endl;
            return 0;
        });
    }

    Usage(argv[0], opts);
    return 0;
}
