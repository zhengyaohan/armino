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

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_R_BASE                  (SOC_TIMER_REG_BASE)

#define TIMER_R_CTRL                  (TIMER_R_BASE + 0x3 * 4)

#define TIMER_F_CLK_DIV               (BIT(3))
#define TIMER_F_CLK_DIV_M             (BIT(3))
#define TIMER_F_CLK_DIV_V             0xf
#define TIMER_F_CLK_DIV_S             0x3

#ifdef __cplusplus
}
#endif

