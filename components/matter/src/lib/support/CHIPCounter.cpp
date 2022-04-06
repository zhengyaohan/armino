/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
 *    All rights reserved.
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

#include <lib/support/CHIPCounter.h>

namespace chip {

MonotonicallyIncreasingCounter::MonotonicallyIncreasingCounter() : mCounterValue(0) {}

MonotonicallyIncreasingCounter::~MonotonicallyIncreasingCounter() {}

CHIP_ERROR
MonotonicallyIncreasingCounter::Init(uint32_t aStartValue)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    mCounterValue = aStartValue;

    return err;
}

CHIP_ERROR
MonotonicallyIncreasingCounter::Advance()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    mCounterValue++;

    return err;
}

uint32_t MonotonicallyIncreasingCounter::GetValue()
{
    return mCounterValue;
}

} // namespace chip
