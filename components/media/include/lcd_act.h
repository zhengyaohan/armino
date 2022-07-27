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
#include <driver/media_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	LCD_STATE_DISABLED,
	LCD_STATE_ENABLED,
} lcd_state_t;


typedef struct
{
	lcd_state_t state;
	uint32_t param;
	uint16_t dec_pixel_x;
	uint8_t *display_address;
	uint8_t *decoder_address;
	uint8_t *rotate_address;
	uint8_t count;
	uint8_t dma_channel;
	frame_buffer_t *frame;
} lcd_info_t;



void lcd_event_handle(uint32_t event, uint32_t param);
lcd_state_t get_lcd_state(void);
void set_lcd_state(lcd_state_t state);
void lcd_init(void);
void lcd_frame_complete_notify(frame_buffer_t *buffer);

#ifdef __cplusplus
}
#endif
