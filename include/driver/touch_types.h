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
#include <driver/int_types.h>


#ifdef __cplusplus
extern "C" {
#endif


#define BK_ERR_TOUCH_ID			(BK_ERR_TOUCH_BASE - 1)


typedef void (*touch_isr_t)(void *param);


typedef enum {
	BK_TOUCH_0 = 0,			/**< touch channel 0 */
	BK_TOUCH_1,				/**< touch channel 1 */
	BK_TOUCH_2,				/**< touch channel 2 */
	BK_TOUCH_3,				/**< touch channel 3 */
	BK_TOUCH_4,				/**< touch channel 4 */
	BK_TOUCH_5,				/**< touch channel 5 */
	BK_TOUCH_6,				/**< touch channel 6 */
	BK_TOUCH_7,				/**< touch channel 7 */
	BK_TOUCH_8,				/**< touch channel 8 */
	BK_TOUCH_9,				/**< touch channel 9 */
	BK_TOUCH_10,			/**< touch channel 10 */
	BK_TOUCH_11,			/**< touch channel 11 */
	BK_TOUCH_12,			/**< touch channel 12 */
	BK_TOUCH_13,			/**< touch channel 13 */
	BK_TOUCH_14,			/**< touch channel 14 */
	BK_TOUCH_15,			/**< touch channel 15 */
	BK_TOUCH_MAX,
} touch_channel_t;

typedef enum {
	BK_SCAN_MODE_TOUCH_0 = 1,					/**< touch scan mode channel 0 */
	BK_SCAN_MODE_TOUCH_1 = 1 << 1,				/**< touch scan mode channel 1 */
	BK_SCAN_MODE_TOUCH_2 = 1 << 2,				/**< touch scan mode channel 2 */
	BK_SCAN_MODE_TOUCH_3 = 1 << 3,				/**< touch scan mode channel 3 */
	BK_SCAN_MODE_TOUCH_4 = 1 << 4,				/**< touch scan mode channel 4 */
	BK_SCAN_MODE_TOUCH_5 = 1 << 5,				/**< touch scan mode channel 5 */
	BK_SCAN_MODE_TOUCH_6 = 1 << 6,				/**< touch scan mode channel 6 */
	BK_SCAN_MODE_TOUCH_7 = 1 << 7,				/**< touch scan mode channel 7 */
	BK_SCAN_MODE_TOUCH_8 = 1 << 8,				/**< touch scan mode channel 8 */
	BK_SCAN_MODE_TOUCH_9 = 1 << 9,				/**< touch scan mode channel 9 */
	BK_SCAN_MODE_TOUCH_10 = 1 << 10,			/**< touch scan mode channel 10 */
	BK_SCAN_MODE_TOUCH_11 = 1 << 11,			/**< touch scan mode channel 11 */
	BK_SCAN_MODE_TOUCH_12 = 1 << 12,			/**< touch scan mode channel 12 */
	BK_SCAN_MODE_TOUCH_13 = 1 << 13,			/**< touch scan mode channel 13 */
	BK_SCAN_MODE_TOUCH_14 = 1 << 14,			/**< touch scan mode channel 14 */
	BK_SCAN_MODE_TOUCH_15 = 1 << 15,			/**< touch scan mode channel 15 */
	BK_SCAN_MODE_TOUCH_MAX,
} touch_scan_mode_channel_t;

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
	TOUCH_DETECT_RANGE_8PF = 0,		/**< 8PF */
	TOUCH_DETECT_RANGE_12PF,		/**< 12PF */
	TOUCH_DETECT_RANGE_19PF,		/**< 19PF */
	TOUCH_DETECT_RANGE_27PF,		/**< 27PF */
} touch_detect_range_t;


typedef struct {
	icu_int_src_t int_src;
	int_group_isr_t isr;
} touch_int_config_t;

typedef struct {
	touch_sensitivity_level_t sensitivity_level;	/**< sensitivity_level */
	touch_detect_threshold_t detect_threshold;		/**< detect_threshold */
	touch_detect_range_t detect_range;				/**< detect_range */
} touch_config_t;


#ifdef __cplusplus
}
#endif


