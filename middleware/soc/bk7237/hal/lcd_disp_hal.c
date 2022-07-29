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

#include <common/bk_include.h>

#if (CONFIG_BK7256XX_MP)
#include "lcd_disp_ll_macro_def_mp2.h"
#else
#include "lcd_disp_ll_macro_def.h"
#endif

#include "lcd_disp_hal.h"

#if (CONFIG_BK7256XX_MP)
/******************************************8080 API********************************************/
void lcd_hal_8080_cmd_send(uint8_t param_count, uint32_t command, uint32_t *param)
{
	int i = 0;

	lcd_disp_ll_set_dat_fifo_thrd_i8080_cmd_para_count(param_count);
	lcd_disp_ll_set_i8080_cmd_fifo_value(command);

	for(i = 0; i < param_count; i++ )
	{
		lcd_disp_ll_set_i8080_cmd_fifo_value ((uint32_t)*(param + i));
	}

	while(lcd_disp_ll_get_disp_status_i8080_cmd_cfg_done() == 0);
	i = 0;
}

void lcd_hal_set_data_fifo_thrd(uint16_t wr_threshold_val, uint16_t rd_threshold_val)
{
	lcd_disp_ll_set_dat_fifo_thrd_dat_wr_thrd(wr_threshold_val);
	lcd_disp_ll_set_dat_fifo_thrd_dat_rd_thrd(rd_threshold_val);
}

void lcd_hal_pixel_config(uint16_t x_pixel, uint16_t y_pixel)
{
	lcd_disp_ll_set_rgb_cfg_x_pixel(x_pixel);
	lcd_disp_ll_set_rgb_cfg_y_pixel(y_pixel);
}

void lcd_hal_set_sync_low(uint8_t hsync_back_low, uint16_t vsync_back_low)
{
	lcd_disp_ll_set_rgb_sync_low_hsync_back_low(hsync_back_low);
	lcd_disp_ll_set_rgb_sync_low_vsync_back_low(vsync_back_low);
}

void lcd_hal_set_partical_display(uint8_t partial_en, uint16_t partial_clum_l, uint16_t partial_clum_r, uint16_t partial_line_l, uint16_t partial_line_r)
{
	lcd_disp_ll_set_rgb_clum_offset_value(partial_clum_l + (partial_clum_r << 16) + (partial_en << 28));
	lcd_disp_ll_set_rgb_line_offset_value(partial_line_l + (partial_line_r << 16));
}


#else
/* wr_threshold_val: 0-0x3ff rd_threshold_val：0-0xfff */
void lcd_hal_rgb_set_thrd(uint16_t wr_threshold_val, uint16_t rd_threshold_val)
{
	lcd_disp_ll_set_display_int_wr_thrd(wr_threshold_val);
	lcd_disp_ll_set_display_int_rd_thrd(rd_threshold_val);
}

void lcd_hal_mem_clr(void)
{
	lcd_disp_ll_set_i8080_config_i8080_fifo_clr(1);
	lcd_disp_ll_set_i8080_config_i8080_cmdfifo_clr(1);
	lcd_disp_ll_set_status_str_fifo_clr(1);
}


void lcd_hal_pixel_config(uint16_t x_pixel, uint16_t y_pixel)
{
	lcd_disp_ll_set_status_x_pixel(x_pixel);
	lcd_disp_ll_set_status_y_pixel(y_pixel);
}

#endif

/******************************************RGB API********************************************/

void lcd_hal_rgb_sync_config(uint16_t rgb_hsync_back_porch, uint16_t rgb_hsync_front_porch, uint16_t rgb_vsync_back_porch, uint16_t rgb_vsync_front_porch)
{
	uint32_t sync = rgb_hsync_back_porch + (rgb_hsync_front_porch << 8) + (rgb_vsync_back_porch << 15) + (rgb_vsync_front_porch << 20);
#if (CONFIG_BK7256XX_MP)
	lcd_disp_ll_set_hsync_vsync_value(sync);
#else
	lcd_disp_ll_set_hsync_vsync_cfg_value(sync);
#endif
}


void lcd_hal_rgb_display_sel(bool en)
{
#if (CONFIG_BK7256XX_MP)
	lcd_disp_ll_set_rgb_cfg_rgb_disp_on(en);
	lcd_disp_ll_set_rgb_cfg_rgb_on(en);
#else
	lcd_disp_ll_set_status_rgb_on(en);
	lcd_disp_ll_set_status_lcd_display_on(en);
#endif
}

/* is_sof_en :1 enable,0:disable ;  is_eof_en:1 enable,0:disable   */
void lcd_hal_rgb_int_enable(bool         is_sof_en, bool is_eof_en)
{
	lcd_disp_ll_set_display_int_rgb_int_en(is_sof_en | (is_eof_en << 1));
}

void lcd_hal_rgb_eof_int_status_clear(void)
{
	lcd_disp_ll_set_display_int_rgb_eof(1);
}

void lcd_hal_rgb_sof_int_status_clear(void)
{
	lcd_disp_ll_set_display_int_rgb_sof(1);
}

/******************************************8080 API********************************************/


/* is_sof_en :1 enable,0:disable ;  is_eof_en:1 enable,0:disable   */
void lcd_hal_8080_int_enable(bool is_sof_en, bool is_eof_en)
{
	lcd_disp_ll_set_display_int_i8080_int_en(is_sof_en| (is_eof_en << 1));
}

void lcd_hal_eof_int_status_clear(void)
{
	lcd_disp_ll_set_display_int_i8080_eof(1);
}
void lcd_hal_sof_int_status_clear(void)
{
	lcd_disp_ll_set_display_int_i8080_sof(1);
}

/* wr_threshold_val: 0-0xff rd_threshold_val：0-0xff */
void lcd_hal_8080_set_fifo_data_thrd(uint16_t wr_threshold_val, uint16_t rd_threshold_val)
{
#if (CONFIG_BK7256XX_MP)
	lcd_disp_ll_set_i8080_thrd_cmd_wr_thrd(wr_threshold_val);
	lcd_disp_ll_set_i8080_thrd_cmd_rd_thrd(rd_threshold_val);
#else
	lcd_disp_ll_set_i8080thrd_dat_wr_thrd(wr_threshold_val);
	lcd_disp_ll_set_i8080thrd_dat_rd_thrd(rd_threshold_val);
#endif
}




