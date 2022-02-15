// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "bk_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ASSERT_TAG "ASSERT"

#if (CONFIG_DEBUG_FIRMWARE)
#define BK_ASSERT(exp)                                       \
do {                                                         \
    if ( !(exp) ) {                                          \
        BK_LOGE(BK_ASSERT_TAG, "%s:%d\r\n", __FUNCTION__, __LINE__); \
        while(1);                                            \
    }                                                        \
} while (0)
#else
#define BK_ASSERT(exp)
#endif

#ifdef __cplusplus
}
#endif
