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

#ifndef __Arguments_h__
#define __Arguments_h__

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

static void Usage(const std::string& program, const std::vector<std::tuple<std::string, std::string>>& opts) {
    std::cout << "usage: " << program << " ";
    std::string actions;
    std::string label;
    for (const auto& opt : opts) {
        if (std::get<0>(opt).size() == 1) {
            label = std::get<1>(opt);
            continue;
        }
        if (label == "Actions") {
            actions += " | --" + std::get<0>(opt);
            continue;
        }
        if (actions != "") {
            std::cout << actions.substr(3) << " ";
            actions = "";
        }
        std::cout << "[--" << std::get<0>(opt) << "] ";
    }
    std::cout << "[files ...]" << std::endl;
}

static auto ParseArguments(int argc, const char** argv, const std::vector<std::tuple<std::string, std::string>>& opts) {
    std::string program(argv[0]);
    std::vector<std::string> args;
    while (argc > 1) {
        args.push_back(std::string(argv[1]));
        --argc;
        ++argv;
    }

    const auto Error = [argv](const std::string& msg) {
        std::cout << argv[0] << ": " << msg << std::endl;
        exit(-1);
    };

    const auto Help = [](auto program, auto opts) {
        Usage(program, opts);
        for (const auto& opt : opts) {
            switch (std::get<0>(opt)[0]) {
                case '=':
                    std::cout << std::endl;
                    std::cout << std::get<1>(opt) << ':' << std::endl;
                    std::cout << std::endl;
                    continue;
                case '*':
                    std::cout << "   " << std::get<1>(opt) << std::endl;
                    continue;
            }
            std::cout << "   --" << std::setw(20) << std::left << std::get<0>(opt) << " " << std::get<1>(opt)
                      << std::endl;
        }
        std::cout << std::endl;
    };

    std::set<std::string> optset;
    std::for_each(opts.begin(), opts.end(), [&optset](auto x) {
        auto name = std::get<0>(x);
        if (name.find(' ') != std::string::npos) {
            name = name.substr(0, name.find(' ')) + " VALUE";
        }
        optset.insert(name);
    });
    std::map<std::string, std::string> optvals;
    std::vector<std::string> optfiles;
    for (auto it = args.begin(); it != args.end(); ++it) {
        auto arg = *it;
        if (arg.front() == '-') {
            while (arg.size() > 1 && arg.front() == '-') {
                arg = arg.substr(1);
            }
            if (optset.find(arg + " VALUE") != optset.end()) {
                if ((it + 1) == args.end()) {
                    Error("option requires an argument -- " + arg);
                }
                optvals[arg] = *++it;
                continue;
            }
            if (optset.find(arg) == optset.end()) {
                Error("illegal option -- " + arg);
            }
            optvals[arg] = "true";
            continue;
        }
        optfiles.push_back(arg);
    }

    if (optvals.find("help") != optvals.end()) {
        Help(program, opts);
        exit(0);
    }

    return std::make_tuple(optvals, optfiles);
}

#endif
