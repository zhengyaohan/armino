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
#include <stdio.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

void f_create_file_to_sdcard(uint32_t frame_id);

void f_write_data_to_sdcard(void *data, uint32_t len, uint8_t is_eof);

void f_close_write_to_sdcard(void);

void bk_video_transfer_image_save_enable(uint8_t enable);

void bk_lcd_video_enable(uint8_t enable);

void bk_lcd_video_blending(uint8_t blend_enable);


#ifdef __cplusplus
}
#endif

