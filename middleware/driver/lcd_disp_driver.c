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
#include "include.h"
#include "bk_api_mem.h"
#include "arch_interrupt.h"
#include "lcd_disp_ll_macro_def.h"
#include "lcd_disp_hal.h"
#include "bk_api_lcd.h"
#include "gpio_map.h"
#include "gpio_driver.h"
#include "bk_api_gpio.h"
#include "gpio_map.h"
#include "bk_api_int.h"
#include "dma_hal.h"
#include "bk_api_dma.h"
#include "dma_driver.h"
#include "sys_driver.h"


bk_err_t gpio_lcd_8080_sel(void)
{
	lcd_gpio_map_t lcd_gpio_map_table[] = GPIO_LCD_8080_GPIO_MAP;
	for (uint32_t i = 0; i < GPIO_LCD_8080_USED_GPIO_NUM; i++) {
		gpio_dev_unmap(lcd_gpio_map_table[i].gpio_id);
		gpio_dev_map(lcd_gpio_map_table[i].gpio_id, lcd_gpio_map_table[i].dev);
	}
	return BK_OK;
}

bk_err_t gpio_lcd_rgb_sel(void)
{
	lcd_gpio_map_t lcd_gpio_map_table[] = GPIO_LCD_RGB_GPIO_MAP;
	for (uint32_t i = 0; i < GPIO_LCD_RGB_GPIO_NUM; i++) {
		gpio_dev_unmap(lcd_gpio_map_table[i].gpio_id);
		gpio_dev_map(lcd_gpio_map_table[i].gpio_id, lcd_gpio_map_table[i].dev);
	}
	return BK_OK;
}


bk_err_t bk_lcd_init(uint8_t clk_src_sel, uint8_t clk_div_l, uint8_t clk_div_h)
{
	/* 0:clk_320M  1:clk_480M */
	/*  Frequency division : F/(1+clkdiv_disp_l+clkdiv_disp_h*2)*/
	sys_drv_lcd_set(clk_src_sel, clk_div_l, clk_div_h, 1, 0);
	return BK_OK;
}

void bk_lcd_8080_reset(void)
{
	hal_lcd_8080_reset();
	hal_lcd_8080_unreset();
}

bk_err_t bk_lcd_deinit(void)
{
	lcd_hal_reg_deinit();
	lcd_hal_8080_int_enable(0, 0);
	lcd_hal_rgb_int_enable(0, 0);
	lcd_hal_8080_display_enable(0);
	lcd_hal_8080_start_transfer(0);
	lcd_hal_rgb_display_en(0);
	return BK_OK;
}


bk_err_t bk_lcd_isr_register(int_isr_t lcd_isr)
{
	bk_int_isr_register(INT_SRC_LCD, lcd_isr, NULL);
	return BK_OK;
}



void bk_lcd_debug(void)
{
	lcd_disp_struct_dump();
}





