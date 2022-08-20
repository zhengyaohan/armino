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
#include "lcd_disp_hal.h"
#include <driver/lcd.h>
#include "gpio_map.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "gpio_map.h"
#include <driver/int.h>
#include "sys_driver.h"
#include <modules/pm.h>
#include <driver/hal/hal_gpio_types.h>
#include <driver/hal/hal_lcd_types.h>
#include "BK7256_RegList.h"
#include "lcd_disp_ll_macro_def_mp2.h"
#include <driver/dma2d.h>
#include <./lcd/lcd_devices.h>


#define IO_FUNCTION_ENABLE(pin, func) 	\
	do {							  	\
		gpio_dev_unmap(pin); 			\
		gpio_dev_map(pin, func);		\
	} while (0)
	
#if (USE_LCD_REGISTER_CALLBACKS == 1)  //register callback
typedef struct {
	lcd_isr_t lcd_8080_frame_start_handler;
	lcd_isr_t lcd_8080_frame_end_handler;
	lcd_isr_t lcd_rgb_frame_end_handler;
	lcd_isr_t lcd_rgb_frame_start_handler;
} lcd_driver_t;
static lcd_driver_t s_lcd = {0};
static void lcd_isr(void);
#endif


static bk_err_t bk_lcd_8080_gpio_init(void)
{
	IO_FUNCTION_ENABLE(LCD_MCU_D0_PIN, LCD_MCU_D0_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_D1_PIN, LCD_MCU_D1_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_D2_PIN, LCD_MCU_D2_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_D3_PIN, LCD_MCU_D3_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_D4_PIN, LCD_MCU_D4_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_D5_PIN, LCD_MCU_D5_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_D6_PIN, LCD_MCU_D6_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_D7_PIN, LCD_MCU_D7_FUNC);

	IO_FUNCTION_ENABLE(LCD_MCU_RDX_PIN, LCD_MCU_RDX_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_WRX_PIN, LCD_MCU_WRX_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_RSX_PIN, LCD_MCU_RSX_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_RESET_PIN, LCD_MCU_RESET_FUNC);
	IO_FUNCTION_ENABLE(LCD_MCU_CSX_PIN, LCD_MCU_CSX_FUNC);

	return BK_OK;
}

static bk_err_t bk_lcd_rgb_gpio_init(void)
{
	IO_FUNCTION_ENABLE(LCD_RGB_R0_PIN, LCD_RGB_R0_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R1_PIN, LCD_RGB_R1_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R2_PIN, LCD_RGB_R2_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R3_PIN, LCD_RGB_R3_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_R4_PIN, LCD_RGB_R4_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G0_PIN, LCD_RGB_G0_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G1_PIN, LCD_RGB_G1_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G2_PIN, LCD_RGB_G2_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G3_PIN, LCD_RGB_G3_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G4_PIN, LCD_RGB_G4_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_G5_PIN, LCD_RGB_G5_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B0_PIN, LCD_RGB_B0_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B1_PIN, LCD_RGB_B1_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B2_PIN, LCD_RGB_B2_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B3_PIN, LCD_RGB_B3_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_B4_PIN, LCD_RGB_B4_FUNC);

	IO_FUNCTION_ENABLE(LCD_RGB_CLK_PIN, LCD_RGB_CLK_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_DISP_PIN, LCD_RGB_DISP_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_HSYNC_PIN, LCD_RGB_HSYNC_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_VSYNC_PIN, LCD_RGB_VSYNC_FUNC);
	IO_FUNCTION_ENABLE(LCD_RGB_DE_PIN, LCD_RGB_DE_FUNC);

	return BK_OK;
}




bk_err_t bk_lcd_8080_send_cmd(uint8_t param_count, uint32_t command, uint32_t *param)
{
	lcd_hal_8080_cmd_send(param_count, command, param);
	return BK_OK;
}


bk_err_t bk_lcd_8080_ram_write(uint32_t command)
{
	lcd_hal_8080_cmd_param_count(1);
	lcd_hal_8080_write_cmd(command);
	return BK_OK;
}

bk_err_t bk_lcd_set_yuv_mode(lcd_data_format_t input_data_format)
{
	switch (input_data_format) 
	{
		case LCD_FMT_RGB565:
			lcd_hal_display_yuv_sel(0);
		break;
		case LCD_FMT_ORGINAL_YUYV:
			lcd_hal_display_yuv_sel(1);
			break;
		case LCD_FMT_UYVY:
			lcd_hal_display_yuv_sel(2);
			break;
		case LCD_FMT_YYUV:
			lcd_hal_display_yuv_sel(3);
			break;
		case LCD_FMT_UVYY:
			lcd_hal_display_yuv_sel(4);
			break;
		case LCD_FMT_VUYY:
			lcd_hal_display_yuv_sel(5);
			break;
		case LCD_FMT_YVYU:
			lcd_hal_display_yuv_sel(1);
			lcd_hal_set_pixel_reverse(1);
			break;
		case LCD_FMT_VYUY:
			lcd_hal_display_yuv_sel(2);
			lcd_hal_set_pixel_reverse(1);
			break;
		case LCD_FMT_YYVU:
			lcd_hal_display_yuv_sel(5);
			lcd_hal_set_pixel_reverse(1);
			break;
		default:
			break;
	}
	return BK_OK;
}

bk_err_t bk_lcd_rgb_init(lcd_device_id_t id, uint16_t x_pixel, uint16_t y_pixel, lcd_data_format_t input_data_format) 
{
	bk_lcd_rgb_gpio_init();
	lcd_hal_rgb_int_enable(0, 1);
	lcd_hal_rgb_display_sel(1);  //RGB display enable, and select rgb module
	lcd_hal_set_sync_low(HSYNC_BACK_LOW, VSYNC_BACK_LOW);
	switch (id)
	{
		case LCD_DEVICE_ST7282:
			lcd_hal_rgb_sync_config(RGB_HSYNC_BACK_PORCH, RGB_HSYNC_FRONT_PORCH, RGB_VSYNC_BACK_PORCH, RGB_VSYNC_FRONT_PORCH);
			lcd_hal_set_rgb_clk_rev_edge(POSEDGE_OUTPUT); 
			break;
		case LCD_DEVICE_HX8282:
			lcd_hal_rgb_sync_config(RGB_720P_HSYNC_BACK_PORCH, RGB_720P_HSYNC_FRONT_PORCH,RGB_720P_VSYNC_BACK_PORCH, RGB_720P_VSYNC_FRONT_PORCH);
			lcd_hal_set_rgb_clk_rev_edge(POSEDGE_OUTPUT); 
			break;
		case LCD_DEVICE_GC9503V:
			lcd_hal_rgb_sync_config(RGB_HSYNC_BACK_PORCH, RGB_HSYNC_FRONT_PORCH, RGB_VSYNC_BACK_PORCH, RGB_VSYNC_FRONT_PORCH);
			lcd_hal_set_rgb_clk_rev_edge(NEGEDGE_OUTPUT);  //output data is in clk doen edge or up adge
			break;
		default:
			lcd_hal_rgb_sync_config(RGB_HSYNC_BACK_PORCH, RGB_HSYNC_FRONT_PORCH, RGB_VSYNC_BACK_PORCH, RGB_VSYNC_FRONT_PORCH);
			lcd_hal_set_rgb_clk_rev_edge(POSEDGE_OUTPUT); 
			break;
	}

	lcd_hal_disconti_mode(DISCONTINUE_MODE);
	bk_lcd_pixel_config(x_pixel, y_pixel);
	bk_lcd_set_yuv_mode(input_data_format);
	lcd_hal_set_data_fifo_thrd(DATA_FIFO_WR_THRD, DATA_FIFO_RD_THRD);
	return BK_OK;
}


bk_err_t bk_lcd_driver_init(lcd_clk_t clk)
{
	bk_err_t ret = BK_OK;

	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
	bk_pm_clock_ctrl(PM_CLK_ID_DISP, CLK_PWR_CTRL_PWR_UP);
	switch(clk) {
		case LCD_320M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_0, DISP_DIV_H_0, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_160M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_0, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_40M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_0, DISP_DIV_H_1, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			//ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_3, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_20M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_7, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			//ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_2, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
		break;
		case LCD_60M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_0, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_80M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_1, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_54M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_2, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_32M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_4, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_12M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_4, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_10M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_5, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_26M:
			ret = sys_drv_lcd_set(DISP_CLK_320M, DISP_DIV_L_1, DISP_DIV_H_5, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		case LCD_8M:
			ret = sys_drv_lcd_set(DISP_CLK_120M, DISP_DIV_L_1, DISP_DIV_H_7, DISP_INT_EN, DSIP_DISCLK_ALWAYS_ON);
			break;
		default:
			break;
	}
	if (ret != BK_OK) {
		os_printf("lcd system reg config error \r\n");
		return BK_FAIL;
	}

#if	(USE_LCD_REGISTER_CALLBACKS == 1) 
	os_memset(&s_lcd, 0, sizeof(s_lcd));
	bk_int_isr_register(INT_SRC_LCD, lcd_isr, NULL);
#endif
	return BK_OK;
}



bk_err_t bk_lcd_8080_init(uint16_t x_pixel, uint16_t y_pixel,lcd_data_format_t input_data_format)
{
	bk_lcd_8080_gpio_init();
	lcd_hal_rgb_display_sel(0); //25bit - rgb_on = 0 select 8080 mode
	lcd_hal_disconti_mode(DISCONTINUE_MODE);
	lcd_hal_8080_verify_1ms_count(VERIFY_1MS_COUNT);
	lcd_hal_8080_set_tik(TIK_CNT);
	lcd_hal_set_data_fifo_thrd(DATA_FIFO_WR_THRD, DATA_FIFO_RD_THRD);
	lcd_hal_8080_set_fifo_data_thrd(CMD_FIFO_WR_THRD,CMD_FIFO_RD_THRD);
	bk_lcd_pixel_config(x_pixel, y_pixel);
	lcd_hal_8080_display_enable(1);
	lcd_hal_8080_int_enable(0, 1); //set eof int enable 
	bk_lcd_set_yuv_mode(input_data_format);
	lcd_hal_8080_sleep_in(1);
	//delay(7017857); //reset need 131ms.
	return BK_OK;
}




bk_err_t bk_lcd_8080_deinit(void)
{

	bk_int_isr_unregister(INT_SRC_LCD);
	lcd_hal_8080_sleep_in(1);
	lcd_hal_8080_int_enable(0, 0);
	lcd_hal_8080_display_enable(0);
	lcd_hal_8080_start_transfer(0);
	bk_pm_clock_ctrl(PM_CLK_ID_DISP, CLK_PWR_CTRL_PWR_DOWN);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
	if (sys_drv_lcd_close() != 0)
	{
		os_printf("lcd system deinit 8080 config error \r\n");
		return BK_FAIL;
	}

	lcd_hal_soft_reset(1);
	rtos_delay_milliseconds(1);
	
	return BK_OK;
}


bk_err_t bk_lcd_rgb_display_en(bool en)
{
	lcd_hal_rgb_display_en(en);
	return BK_OK;
}

bk_err_t bk_lcd_rgb_deinit(void)
{
	lcd_hal_rgb_int_enable(0, 0);
	lcd_hal_rgb_display_en(0);
	lcd_hal_rgb_display_sel(0);
	bk_int_isr_unregister(INT_SRC_LCD);
	bk_pm_clock_ctrl(PM_CLK_ID_DISP, CLK_PWR_CTRL_PWR_DOWN);
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
	if(sys_drv_lcd_close() != 0) {
		os_printf("lcd system deinit reg config error \r\n");
		return BK_FAIL;
	}
	lcd_hal_soft_reset(1);
	rtos_delay_milliseconds(1);
	return BK_OK;
}

//static uint32_t bk_lcd_get_status(void)
//{
//	return reg_DISP_STATUS;
//}

bk_err_t bk_lcd_8080_start_transfer(bool start)
{
	lcd_hal_8080_start_transfer(start);
	return BK_OK;
}


#if (USE_LCD_REGISTER_CALLBACKS == 1)  //register callback
static void lcd_isr(void)
{
	uint32_t int_status = lcd_hal_int_status_get();

	if (int_status & RGB_OUTPUT_SOF)
	{
		if (s_lcd.lcd_rgb_frame_start_handler)
		{
			s_lcd.lcd_rgb_frame_start_handler();
		}
		lcd_hal_rgb_sof_int_status_clear();
	}
	if (int_status & RGB_OUTPUT_EOF)
	{
		if (s_lcd.lcd_rgb_frame_end_handler)
		{
			s_lcd.lcd_rgb_frame_end_handler();
		}
		lcd_hal_rgb_eof_int_status_clear();
	}
	if (int_status & I8080_OUTPUT_SOF)
	{
		if (s_lcd.lcd_8080_frame_start_handler)
		{
			s_lcd.lcd_8080_frame_start_handler();
		}
		lcd_hal_eof_int_status_clear();
	}

	if (int_status & I8080_OUTPUT_EOF)
	{
		if (s_lcd.lcd_8080_frame_end_handler)
		{
			s_lcd.lcd_8080_frame_end_handler();
		}
		lcd_hal_eof_int_status_clear();
	}
}

bk_err_t  bk_lcd_isr_register(lcd_int_type_t int_type, lcd_isr_t isr)
{
	if(int_type == I8080_OUTPUT_SOF) {
		s_lcd.lcd_8080_frame_start_handler = isr;
	}
	if(int_type == I8080_OUTPUT_EOF) {
		s_lcd.lcd_8080_frame_end_handler = isr;
	}
	if(int_type == RGB_OUTPUT_SOF) {
		s_lcd.lcd_rgb_frame_start_handler= isr;
	}
	if(int_type == RGB_OUTPUT_EOF) {
		s_lcd.lcd_rgb_frame_end_handler= isr;
	}
	return BK_OK;
}
#endif

static  void dma2d_lcd_fill(uint32_t frameaddr, uint16_t width, uint16_t height, uint32_t color)
{
	dma2d_config_t dma2d_config = {0};

	dma2d_config.init.mode   = DMA2D_R2M;                      /**< Mode Register to Memory */
	dma2d_config.init.color_mode	   = DMA2D_OUTPUT_ARGB8888;  /**< DMA2D Output color mode is ARGB4444 (16 bpp) */
	dma2d_config.init.output_offset  = 0;                      /**< offset in output */
	dma2d_config.init.red_blue_swap   = DMA2D_RB_REGULAR;       /**< No R&B swap for the output image */
	dma2d_config.init.alpha_inverted = DMA2D_REGULAR_ALPHA;     /**< No alpha inversion for the output image */
	bk_dma2d_driver_init(&dma2d_config);

	if (width == 0 && height == 0)
	{
	}
	else if (width == 0)
	{
		height = height/2;
	} 
	else if(height == 0)
	{
		width = width/2;
	}
	else
	{
		width = width/2;
	}
	bk_dma2d_start_transfer(&dma2d_config, color, (uint32_t)frameaddr, width, height);
	while (bk_dma2d_is_transfer_busy()) {
	}
}
bk_err_t bk_lcd_fill_color(lcd_device_id_t id, lcd_disp_framebuf_t *lcd_disp, uint32_t color)
{
	dma2d_lcd_fill((uint32_t)lcd_disp->buffer, lcd_disp->rect.width, lcd_disp->rect.height, color);
	lcd_hal_pixel_config(lcd_disp->rect.width, lcd_disp->rect.height);
	//os_printf("displat partical (%d, %d)\r\n", lcd_disp->rect.width, lcd_disp->rect.height);
	switch (id)
	{
		case LCD_DEVICE_ST7796S:
			lcd_st7796s_set_display_mem_area(lcd_disp->rect.x+1, lcd_disp->rect.x + lcd_disp->rect.width , lcd_disp->rect.y+1, lcd_disp->rect.y + lcd_disp->rect.height);
			//os_printf("lcd partical  (xs, xe ,ys, ye) = (%d, %d,%d, %d)\r\n", lcd_disp->rect.x+1, lcd_disp->rect.x + lcd_disp->rect.width , lcd_disp->rect.y+1, lcd_disp->rect.y + lcd_disp->rect.height);
			break;
		default:
			break;
	}
	lcd_driver_set_display_base_addr((uint32_t)lcd_disp->buffer);
	return BK_OK;
}

bk_err_t bk_lcd_fill_data(lcd_device_id_t id, lcd_disp_framebuf_t *lcd_disp)
{
	lcd_hal_pixel_config(lcd_disp->rect.width, lcd_disp->rect.height);
	switch (id)
	{
		case LCD_DEVICE_ST7796S:
			lcd_st7796s_set_display_mem_area(lcd_disp->rect.x+1, lcd_disp->rect.x + lcd_disp->rect.width , lcd_disp->rect.y+1, lcd_disp->rect.y + lcd_disp->rect.height);
			break;
		default:
			break;
	}
	lcd_driver_set_display_base_addr((uint32_t)lcd_disp->buffer);
	return BK_OK;
}

bk_err_t bk_lcd_draw_point(lcd_device_id_t id, lcd_rect_t *rect, uint32_t *buf)
{
	uint32_t size = rect->width * rect->height;
	//os_printf("size (%p)\r\n", size);
	
	switch (id)
	{
		case LCD_DEVICE_ST7796S:
			lcd_st7796s_set_display_mem_area(rect->x + 1, rect->x + rect->width, rect->y + 1, rect->y + rect->height);
			break;
		default:
			break;
	}
	bk_lcd_8080_send_cmd(size, 0x2c, buf);
	return BK_OK;
}



