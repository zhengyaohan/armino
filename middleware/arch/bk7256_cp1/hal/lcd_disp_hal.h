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

#include "hal_config.h"
#include "lcd_disp_hw.h"
#include "lcd_disp_ll.h"
#include "lcd_disp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	lcd_disp_hw_t *hw;
} lcd_disp_hal_t;


#define lcd_hal_trans_tik_cnt(hal, tik_cnt) lcd_display_ll_8080_tik_set((hal)->hw, tik_cnt)
#define lcd_hal_int_status_get(hal)  lcd_display_ll_int_status_get((hal)->hw)
#define lcd_hal_int_status_clear(hal, int_type) lcd_display_ll_clear_int_status((hal)->hw, int_type)	
#define lcd_hal_int_enable(hal, int_type, en)   lcd_display_ll_int_en((hal)->hw, int_type, en)
#define lcd_hal_pixel_config(hal, x_pixel, y_pixel)  lcd_display_ll_pixel_config((hal)->hw, x_pixel, y_pixel)
#define lcd_hal_fifo_clr(hal)  lcd_display_fifo_clr((hal)->hw)
#define lcd_hal_fifo_mode(hal, fifo_mode) lcd_display_ll_8080_fifo_mode((hal)->hw, fifo_mode)
#define lcd_hal_rgb_clk_div(hal, clk_div) lcd_display_ll_rgb_clk_div((hal)->hw, clk_div)
#define lcd_hal_rgb_dclk_rev_edge(hal, edge) lcd_display_ll_rgb_dclk_rev((hal)->hw, edge)
#define lcd_hal_rgb_hsync(hal, hsync_back_porch, hsync_front_porch) lcd_display_ll_rgb_hsync((hal)->hw, hsync_back_porch, hsync_front_porch)
#define lcd_hal_rgb_vsync(hal, vsync_back_porch, vsync_front_porch) lcd_display_ll_rgb_vsync((hal)->hw, vsync_back_porch, vsync_front_porch)
#define lcd_hal_rgb_yuv_sel(hal, data_format) lcd_display_ll_rgb_yuv_sel((hal)->hw, data_format)
#define lcd_hal_rgb_disconti_mode(hal, en) lcd_display_ll_rgb_disconti_mode((hal)->hw, en)
#define lcd_hal_rgb_display_open(hal, en)  lcd_display_ll_rgb_display_on((hal)->hw, en)

bk_err_t lcd_hal_init(lcd_disp_hal_t *hal);

bk_err_t lcd_hal_display_enable(lcd_disp_hal_t *hal, bool en);
bk_err_t lcd_hal_data_transfer_start(lcd_disp_hal_t *hal, bool start_transfer);
bk_err_t lcd_hal_set_thrd(lcd_disp_hal_t *hal, uint32_t thrd_type, uint32_t threshold_val);
bk_err_t lcd_hal_write_cmd(lcd_disp_hal_t *hal, uint16_t cmd);
bk_err_t lcd_hal_write_data(lcd_disp_hal_t *hal, uint16_t data);
bk_err_t lcd_hal_reset_befor_cmd_init(lcd_disp_hal_t *hal);


#if CFG_HAL_DEBUG_LCD_DISP
void lcd_disp_struct_dump(void);
#else
#define lcd_disp_struct_dump()
#endif

#ifdef __cplusplus
}
#endif


