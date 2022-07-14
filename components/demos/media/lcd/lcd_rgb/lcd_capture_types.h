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

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	uint16_t x_pixel;
	uint16_t y_pixel;
} lcd_capture_setup_t;

/* used in cpu0 */
typedef enum {
	LCD_IDLE = 0,
	LCD_CAPTURE,
	LCD_EXIT,
	LCD_MAX,
} lcd_capyure_opcode_t;

typedef struct {
	lcd_capyure_opcode_t op;
} lcd_capture_msg_t;

#ifdef __cplusplus
}
#endif
