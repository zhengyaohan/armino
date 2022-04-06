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

#include <stdlib.h>
#include <common/bk_include.h>
#include <os/mem.h>
#include "arch_interrupt.h"
#include "lcd_disp_ll_macro_def.h"
#include "lcd_disp_hal.h"
#include <driver/lcd.h>
#include "gpio_map.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "gpio_map.h"
#include <driver/int.h>
#include "dma_hal.h"
#include <driver/dma.h>
#include "dma_driver.h"
#include "sys_driver.h"

static bk_err_t bk_lcd_8080_gpio_init(void)
{
	lcd_gpio_map_t lcd_gpio_map_table[] = GPIO_LCD_8080_GPIO_MAP;
	for (uint32_t i = 0; i < GPIO_LCD_8080_USED_GPIO_NUM; i++) {
		gpio_dev_unmap(lcd_gpio_map_table[i].gpio_id);
		gpio_dev_map(lcd_gpio_map_table[i].gpio_id, lcd_gpio_map_table[i].dev);
	}
	return BK_OK;
}

static bk_err_t bk_lcd_rgb_gpio_init(void)
{
	lcd_gpio_map_t lcd_gpio_map_table[] = GPIO_LCD_RGB_GPIO_MAP;
	for (uint32_t i = 0; i < GPIO_LCD_RGB_GPIO_NUM; i++) {
		gpio_dev_unmap(lcd_gpio_map_table[i].gpio_id);
		gpio_dev_map(lcd_gpio_map_table[i].gpio_id, lcd_gpio_map_table[i].dev);
	}
	return BK_OK;
}


bk_err_t bk_lcd_sysclk_init(uint8_t clk_src_sel, uint8_t clk_div_l, uint8_t clk_div_h)
{
	/* 0:clk_320M  1:clk_480M */
	/*  Frequency division : F/(1+clkdiv_disp_l+clkdiv_disp_h*2)*/
	sys_drv_lcd_set(clk_src_sel, clk_div_l, clk_div_h, 1, 0);
	return BK_OK;
}

static void bk_lcd_8080_reset(void)
{
	hal_lcd_8080_reset();
	hal_lcd_8080_unreset();
}



void bk_lcd_8080_init(DISPLAY_PIXEL_FORMAT x_pixel, DISPLAY_PIXEL_FORMAT y_pixel)
{
	bk_lcd_8080_gpio_init();
	lcd_hal_mem_clr();
	lcd_hal_disconti_mode(1);
	lcd_hal_8080_set_fifo_data_thrd(96,96);
	bk_lcd_pixel_config(x_pixel, y_pixel);
	lcd_hal_8080_display_enable(1);
	lcd_hal_8080_display_enable(0);
	bk_lcd_8080_reset();
}

void bk_lcd_rgb_init(uint32_t clk_div, DISPLAY_PIXEL_FORMAT x_pixel, DISPLAY_PIXEL_FORMAT y_pixel, uint8_t input_data_format) 
{
	bk_lcd_rgb_gpio_init();
	lcd_hal_mem_clr();
	lcd_hal_set_rgb_clk_div(clk_div);
	bk_lcd_pixel_config(x_pixel, y_pixel);
	lcd_hal_rgb_display_sel(1);
	lcd_hal_rgb_set_thrd(0x60, 0x60);
	lcd_hal_rgb_sync_config(hsync_back_porch, hsync_front_porch, vsync_back_porch, vsync_front_porch);
	lcd_hal_rgb_yuv_sel(input_data_format);
}

bk_err_t bk_lcd_deinit(void)
{
	bk_lcd_8080_reset();
	lcd_hal_8080_int_enable(0, 0);
	lcd_hal_8080_display_enable(0);
	lcd_hal_8080_start_transfer(0);
	lcd_hal_pixel_config(0,0);
	lcd_hal_rgb_int_enable(0, 0);
	lcd_hal_rgb_display_en(0);
	lcd_hal_rgb_display_sel(0);
	lcd_hal_mem_clr();
	bk_lcd_isr_unregister();
	return BK_OK;
}


bk_err_t bk_lcd_isr_register(int_isr_t lcd_isr)
{
	bk_int_isr_register(INT_SRC_LCD, lcd_isr, NULL);
	return BK_OK;
}


bk_err_t bk_lcd_isr_unregister(void)
{
	bk_int_isr_unregister(INT_SRC_LCD);
	return BK_OK;
}






