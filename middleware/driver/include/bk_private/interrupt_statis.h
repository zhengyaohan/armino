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
#include "bk_api_int_types.h"
#if CONFIG_INT_STATIS
int_statis_t g_int_statis_num = {0};
#else
#endif

#if CONFIG_INT_STATIS
#define INT_STATIS_INC((_x)) (_x)++
#else
#define INT_STATIS_INC((_x))
#endif
