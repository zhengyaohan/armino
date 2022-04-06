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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>

#include "JLink.h"
#include "Loader.h"

// https://en.wikipedia.org/wiki/Intel_HEX
typedef enum {
    IHEX_DATA = 0x00,
    IHEX_EOF = 0x01,
    IHEX_EXTENDED_LINEAR_ADDRESS = 0x04,
} IHEXRecordType;

void LoadIHEX(const std::string& file, std::vector<std::pair<TargetAddress, std::vector<uint8_t>>>& result) {
    std::stringstream ss(file);
    std::string line;
    TargetAddress base = 0;
    size_t start_index = result.size();
    while (std::getline(ss, line)) {
        assert(line.front() == ':');
        assert(line.back() == '\r');
        line = line.substr(1, line.size() - 2);
        assert((line.size() % 2) == 0);
        // decode hex record
        std::list<uint8_t> record;
        for (size_t i = 0; i < line.size(); i += 2) {
            record.push_back(std::stoul(line.substr(i, 2), nullptr, 16));
        }
        // calculate checksum
        assert(record.size() > 1);
        auto csum = record.back();
        record.pop_back();
        uint8_t sum = 0;
        for (const auto v : record) {
            sum += v;
        }
        assert(csum == uint8_t(-sum));
        // data length
        uint8_t length = record.front();
        record.pop_front();
        // 16-bit address
        uint16_t address = record.front();
        record.pop_front();
        address <<= 8;
        address |= record.front();
        record.pop_front();
        uint8_t type = record.front();
        record.pop_front();
        assert(record.size() == length);
        std::vector<uint8_t> data(record.begin(), record.end());
        switch (type) {
            case IHEX_DATA: {
                TargetAddress vaddr = base + address;
                if (!result.size() || (result.back().first + result.back().second.size()) != vaddr) {
                    result.push_back(std::make_pair(vaddr, std::vector<uint8_t>()));
                }
                auto& mem(result.back());
                assert(mem.first + mem.second.size() == vaddr);
                size_t end = mem.second.size();
                mem.second.resize(end + data.size());
                memcpy(mem.second.data() + end, data.data(), data.size());
                break;
            }
            case IHEX_EOF:
                std::cout << "Addr       Size" << std::endl;
                for (size_t i = start_index; i < result.size(); ++i) {
                    const auto& block(result[i]);
                    std::cout << std::hex;
                    std::cout << "0x" << std::setw(8) << std::setfill('0') << block.first;
                    std::cout << " 0x" << std::setw(8) << std::setfill('0') << block.second.size();
                    std::cout << std::endl;
                }
                return;
            case IHEX_EXTENDED_LINEAR_ADDRESS:
                assert(data.size() == 2);
                base = (TargetAddress(data[0]) << 24) | (TargetAddress(data[1]) << 16);
                break;
            default:
                std::cout << (int) type << std::endl;
                assert(false);
        }
    }
    assert(false);
}

static std::map<uint32_t, std::string> pt_types = {
    { PT_NULL, "NULL    " },
    { PT_LOAD, "LOAD    " },
    { PT_LOPROC + 1, "EXIDX   " },
};

void LoadELF(
        const std::string& file,
        std::vector<std::pair<TargetAddress, std::vector<uint8_t>>>& result,
        std::map<std::string, TargetAddress>& symtab) {
    const uint8_t* start = (const uint8_t*) file.c_str();
    const Elf_Ehdr* hdr = (Elf_Ehdr*) start;
    const Elf_Shdr* shdr = (Elf_Shdr*) (start + hdr->e_shoff);
    const Elf_Phdr* phdr = (Elf_Phdr*) (start + hdr->e_phoff);
    assert(hdr->e_type == ET_EXEC);
    uint32_t text_base = 0xffffffffU;
    for (size_t n = 0; n < hdr->e_shnum; ++n) {
        const Elf_Shdr& section = shdr[n];
        std::string name((const char*) start + shdr[hdr->e_shstrndx].sh_offset + section.sh_name);
        if (section.sh_type == SHT_SYMTAB) {
            const char* strings = (const char*) (start + shdr[section.sh_link].sh_offset);
            const Elf_Sym* syms = (Elf_Sym*) (start + section.sh_offset);
            for (size_t i = 0; i < section.sh_size / sizeof(Elf_Sym); ++i) {
                symtab[std::string(strings + syms[i].st_name)] = syms[i].st_value;
            }
            continue;
        }
        if (section.sh_type != SHT_PROGBITS || !(section.sh_flags & SHF_ALLOC) || name == ".data") {
            continue;
        }
        text_base = std::min(section.sh_addr, text_base);
    }
    std::cout << std::hex << "ELF code start: " << text_base << std::endl;
    std::cout << "Type     Offset     VirtAddr   PhysAddr   FileSize   MemSize" << std::endl;
    for (size_t n = 0; n < hdr->e_phnum; ++n) {
        const Elf_Phdr& segment = phdr[n];
        std::cout << pt_types[segment.p_type];
        std::cout << std::hex;
        std::cout << " 0x" << std::setw(8) << std::setfill('0') << segment.p_offset;
        std::cout << " 0x" << std::setw(8) << std::setfill('0') << segment.p_vaddr;
        std::cout << " 0x" << std::setw(8) << std::setfill('0') << segment.p_paddr;
        std::cout << " 0x" << std::setw(8) << std::setfill('0') << segment.p_filesz;
        std::cout << " 0x" << std::setw(8) << std::setfill('0') << segment.p_memsz;
        if (segment.p_type != PT_LOAD || !segment.p_filesz) {
            std::cout << " (ignored)" << std::endl;
            continue;
        }
        std::cout << std::endl;
        TargetAddress addr = segment.p_paddr;
        TargetWord memsz = segment.p_memsz;
        size_t filesz = segment.p_filesz;
        size_t fileoffset = segment.p_offset;
        assert(memsz >= filesz);
        if (addr < text_base) {
            TargetWord skip = text_base - addr;
            addr += skip;
            memsz -= skip;
            filesz -= skip;
            fileoffset += skip;
        }
        std::vector<uint8_t> mem;
        mem.resize(filesz);
        assert(file.begin() + fileoffset < file.end());
        assert(file.begin() + fileoffset + filesz <= file.end());
        memcpy(mem.data(), start + fileoffset, filesz);
        result.push_back(std::make_pair(addr, mem));
    }
}
