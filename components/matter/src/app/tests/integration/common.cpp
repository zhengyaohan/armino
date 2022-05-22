/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements constants, globals and interfaces common
 *      to and used by CHIP example applications.
 *
 */

#include <errno.h>

#include <app/tests/integration/common.h>
#include <lib/core/CHIPCore.h>
#include <lib/core/CHIPTLVDebug.hpp>
#include <lib/support/EnforceFormat.h>
#include <lib/support/ErrorStr.h>
#include <lib/support/logging/Constants.h>
#include <platform/CHIPDeviceLayer.h>

chip::FabricTable gFabricTable;
chip::Messaging::ExchangeManager gExchangeManager;
chip::SessionManager gSessionManager;
chip::secure_channel::MessageCounterManager gMessageCounterManager;
chip::SessionHolder gSession;
chip::TestPersistentStorageDelegate gStorage;

void InitializeChip(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    printf("Init CHIP Stack\r\n");

    // Initialize System memory and resources
    err = chip::Platform::MemoryInit();
    SuccessOrExit(err);

    // Initialize the CHIP stack.
    err = chip::DeviceLayer::PlatformMgr().InitChipStack();
    SuccessOrExit(err);

exit:
    if (err != CHIP_NO_ERROR)
    {
        printf("Failed to init CHIP Stack with err: %s\r\n", chip::ErrorStr(err));
        exit(EXIT_FAILURE);
    }
}

void ShutdownChip(void)
{
    gMessageCounterManager.Shutdown();
    gExchangeManager.Shutdown();
    gSessionManager.Shutdown();
    chip::DeviceLayer::PlatformMgr().Shutdown();
}

void ENFORCE_FORMAT(1, 2) TLVPrettyPrinter(const char * aFormat, ...)
{
    va_list args;

    va_start(args, aFormat);

    vprintf(aFormat, args);

    va_end(args);
}
