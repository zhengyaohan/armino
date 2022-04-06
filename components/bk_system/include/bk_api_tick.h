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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int bk_tick_init(void);
int bk_tick_reload(uint32_t time_ms);
void bk_tick_handle(uint8_t arg);
int bk_update_tick(uint32_t tick);
uint64_t bk_get_tick(void);
uint32_t bk_get_second(void);
uint32_t bk_get_ms_per_tick(void);
uint32_t bk_get_ticks_per_second(void);
int bk_get_tick_timer_id(void);

#ifdef __cplusplus
}
#endif
