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


#include <common/bk_include.h>
#include "arch_interrupt.h"
#include <driver/int_types.h>


#ifdef __cplusplus
extern "C" {
#endif


#define BK_ERR_TOUCH_ID			(BK_ERR_TOUCH_BASE - 1)


typedef void (*touch_isr_t)(void *param);


typedef enum {
	BK_TOUCH_0 = 0,
	BK_TOUCH_1,
	BK_TOUCH_2,
	BK_TOUCH_3,
	BK_TOUCH_4,
	BK_TOUCH_5,
	BK_TOUCH_6,
	BK_TOUCH_7,
	BK_TOUCH_8,
	BK_TOUCH_9,
	BK_TOUCH_10,
	BK_TOUCH_11,
	BK_TOUCH_12,
	BK_TOUCH_13,
	BK_TOUCH_14,
	BK_TOUCH_15,
	BK_TOUCH_MAX,
} touch_channel_t;

typedef enum {
	TOUCH_SENSITIVITY_LEVLE_0 = 0,
	TOUCH_SENSITIVITY_LEVLE_1,
	TOUCH_SENSITIVITY_LEVLE_2,
	TOUCH_SENSITIVITY_LEVLE_3,
} touch_sensitivity_level_t;

typedef enum {
	TOUCH_DETECT_THRESHOLD_0 = 0,
	TOUCH_DETECT_THRESHOLD_1,
	TOUCH_DETECT_THRESHOLD_2,
	TOUCH_DETECT_THRESHOLD_3,
	TOUCH_DETECT_THRESHOLD_4,
	TOUCH_DETECT_THRESHOLD_5,
	TOUCH_DETECT_THRESHOLD_6,
	TOUCH_DETECT_THRESHOLD_7,
} touch_detect_threshold_t;

typedef enum {
	TOUCH_DETECT_RANGE_8PF = 0,
	TOUCH_DETECT_RANGE_12PF,
	TOUCH_DETECT_RANGE_19PF,
	TOUCH_DETECT_RANGE_27PF,
} touch_detect_range_t;


typedef struct {
	icu_int_src_t int_src;
	int_group_isr_t isr;
} touch_int_config_t;

typedef struct {
	touch_sensitivity_level_t sensitivity_level;
	touch_detect_threshold_t detect_threshold;
	touch_detect_range_t detect_range;
} touch_config_t;


#ifdef __cplusplus
}
#endif


