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

#include <components/log.h>

#define TOUCH_TAG "touch"
#define TOUCH_LOGI(...) BK_LOGI(TOUCH_TAG, ##__VA_ARGS__)
#define TOUCH_LOGW(...) BK_LOGW(TOUCH_TAG, ##__VA_ARGS__)
#define TOUCH_LOGE(...) BK_LOGE(TOUCH_TAG, ##__VA_ARGS__)
#define TOUCH_LOGD(...) BK_LOGD(TOUCH_TAG, ##__VA_ARGS__)

#define SOC_TOUCH_ID_NUM		16

void touch_isr(void);

