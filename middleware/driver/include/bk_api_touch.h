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

#include "bk_include.h"
#include "bk_api_touch_type.h"

#ifdef __cplusplus
extern "C" {
#endif


bk_err_t bk_touch_gpio_init(touch_channel_t touch_id);
bk_err_t bk_touch_enable(touch_channel_t touch_id);
bk_err_t bk_touch_disable(void);
bk_err_t bk_touch_config(const touch_config_t *touch_config);
bk_err_t bk_touch_calib_enable(uint32_t enable);
bk_err_t bk_touch_scan_mode_enable(uint32_t enable);
bk_err_t bk_touch_manul_mode_enable(uint32_t calib_value);
bk_err_t bk_touch_manul_mode_disable(void);
bk_err_t bk_touch_scan_mode_mult_channl_set(uint32_t chann_value);
bk_err_t bk_touch_int_enable(touch_channel_t touch_id, uint32_t enable);
bk_err_t bk_touch_get_int_status(void);
bk_err_t bk_touch_clear_int(touch_channel_t touch_id);
bk_err_t bk_touch_get_calib_value(void);
bk_err_t bk_touch_get_touch_status(void);
bk_err_t bk_touch_register_touch_isr(touch_channel_t touch_id, touch_isr_t isr, void *param);


#ifdef __cplusplus
}
#endif


