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

#if (HAP_TESTING == 1)
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "HAP.h"

#include "ADK.h"
#include "AppBase.h"
#include "ApplicationFeatures.h"

#if (HAP_TESTING == 1)

static struct adk_test_opts _opts;

static __inline void CopyStringArg(char* inStr, char** outStr) {
    int len = strlen(inStr) + 1;
    *outStr = calloc(len, sizeof(char));
    HAPRawBufferCopyBytes(*outStr, inStr, len);
}

static void PrintCommandLineUsage(char* progname) {
    // clang-format off
    printf(
        "Usage:\n"
        "   %s [options]\n"
        "\n"
        "Options:\n"
        "  -m PATH, --media-path PATH\n"
        "           The path for Media files (Darwin Only)\n"
        "  -a ACCESSORYNAME, --accessory-name ACCESSORYNAME\n"
        "           The new name of the accessory.\n"
        "  -V, --version\n"
        "           Prints the version of the APP.\n"
        "  -v FIRMWAREVERSION, --firmware-version FIRMWAREVERSION (ex: \"1\", \"2.1.3\")\n"
        "           Override firmware version defined by application.\n"
#if (HAVE_NFC_ACCESS == 1)
        "  -r HARDWAREFINISH, --hardware-finish HARDWAREFINISH\n"
        "           Override the hardware finish defined by application.\n"
        "           e.g. --hardware-finish 0xECD6AA\n"
#endif
        "  -c COMMANDFILEPATH, --commandfile-path COMMANDFILEPATH \n"
        "           The path for the command line file. Default value is .command.input"
        "           in the directory where the accessory is started.\n"
        "           e.g. --commandfile-path /tmp/commands.txt\n"
#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
        "  -H, --hds-over-hap\n"
        "           Override Data Stream to use HAP transport (default is TCP)\n"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
        "  -f, --fwup-persist-staging\n"
        "           Use persistent storage for firmware update staging status\n"
#endif
        "  -k HOMEKITSTOREPATH, --homekitstorepath HOMEKITSTOREPATH\n"
        "           The path for the Home Kit store\n "
        "           e.g. --homekitstorepath ./.HomeKitStoreAbc\n"
        "  --suppress-unpaired-thread-advertising\n"
        "           Whether or not to suppress unpaired thread advertisements (default: 0)"
        "  -h, --help\n"
        "           Print this usage.\n"
        "\n"
        , progname);
    // clang-format on
}

#define SUPPRESS_UNPAIRED_THREAD_ADVERTISEMENTS_VALUE 1000

static void SetupCommandLineArgs(int argc, char* argv[]) {
    struct option long_options[] = {
        { "help", no_argument, NULL, 'h' },
        { "media-path", required_argument, NULL, 'm' },
        { "accessory-name", required_argument, NULL, 'a' },
        { "version", no_argument, NULL, 'V' },
        { "firmware-version", required_argument, NULL, 'v' },
#if (HAVE_NFC_ACCESS == 1)
        { "hardware-finish", required_argument, NULL, 'r' },
#endif
        { "commandfile-path", required_argument, NULL, 'c' },
#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
        { "hds-over-hap", no_argument, NULL, 'H' },
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
        { "fwup-persist-staging", no_argument, NULL, 'f' },
#endif
        { "homekitstorepath", required_argument, NULL, 'k' },
        { "suppress-unpaired-thread-advertising",
          required_argument,
          NULL,
          SUPPRESS_UNPAIRED_THREAD_ADVERTISEMENTS_VALUE },
        { NULL, 0, NULL, 0 },
    };

    const char* const argSpec =
            "p:"
            "m:"
            "a:"
            "v:"
#if (HAVE_NFC_ACCESS == 1)
            "r:"
#endif
            "k:"
            "c:"
            "V:"
#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
            "H"
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
            "f"
#endif
            "h";

    int opt;
    int long_index = 0;
    _opts.suppressUnpairedThreadAdvertisements = false;

    if (!argv) {
        return;
    }

    while ((opt = getopt_long(argc, argv, argSpec, long_options, &long_index)) != -1) {
        switch (opt) {
            case 'm': {
                CopyStringArg(optarg, &_opts.media_path);
                break;
            }
            case 'a': {
                CopyStringArg(optarg, &_opts.accessory_name);
                break;
            }
            case 'k': {
                CopyStringArg(optarg, &_opts.homeKitStorePath);
                break;
            }
            case 'v': {
                CopyStringArg(optarg, &_opts.firmware_version);
                break;
            }
#if (HAVE_NFC_ACCESS == 1)
            case 'r': {
                _opts.hardwareFinish = strtoul(optarg, NULL, 16);
                break;
            }
#endif
            case 'c': {
                CopyStringArg(optarg, &_opts.commandFilePath);
                break;
            }
            case 'V': {
                printf("ADK Version: %s (%s) - compatibility version %lu\n",
                       HAPGetVersion(),
                       HAPGetBuild(),
                       (unsigned long) HAPGetCompatibilityVersion());
                exit(0);
            }
#if (HAVE_HDS_TRANSPORT_OVER_HAP == 1)
            case 'H': {
                _opts.hds_over_hap_override = true;
                break;
            }
#endif
#if (HAVE_FIRMWARE_UPDATE == 1)
            case 'f': {
                _opts.fwup_persist_staging = true;
                break;
            }
#endif
                // dump out command line args
            case SUPPRESS_UNPAIRED_THREAD_ADVERTISEMENTS_VALUE: {
                _opts.suppressUnpairedThreadAdvertisements = (atoi(optarg) == 0 ? false : true);
                break;
            }
            // dump out command line args
            case '?':
            case 'h':
            default: {
                PrintCommandLineUsage(argv[0]);
                exit(0);
            }
        }
    }
}

#define FREEIF(X) \
    if ((X)) { \
        free((X)); \
    }

void FreeAdkTestOpts() {
    FREEIF(_opts.media_path)
    FREEIF(_opts.accessory_name)
    FREEIF(_opts.firmware_version)
    FREEIF(_opts.homeKitStorePath)
    FREEIF(_opts.commandFilePath)
}

#endif

void stop(void) {
    AdkStopApplication();
}

int main(int argc HAP_UNUSED, char* _Nullable argv[_Nullable] HAP_UNUSED) {
    void* ctx = NULL;

#if (HAP_TESTING == 1)
    SetupCommandLineArgs(argc, argv);
    ctx = &_opts;

#endif

    (void) AdkRunApplication(ctx);
    // We don't expect to come out of this function

#if (HAP_TESTING == 1)
    FreeAdkTestOpts();
#endif

    return 0;
}
