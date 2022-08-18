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
#include "dma_hal.h"
#include <driver/dma.h>
#include "dma_driver.h"
#include "sys_driver.h"
#include "jpeg_dec_macro_def.h"
#include "jpeg_dec_ll_macro_def.h"
#include "jpeg_dec_hal.h"
#include <driver/jpeg_dec.h>
#include "driver/jpeg_dec_types.h"
#include <driver/hal/hal_jpeg_dec_types.h>
#include <modules/pm.h>
#include "jpeg_dec_ll_macro_def.h"

#if (USE_JPEG_DEC_COMPLETE_CALLBACKS == 1)
jpeg_dec_isr_cb_t  s_jpeg_dec_isr[DEC_ISR_MAX] = {NULL};
static jpeg_dec_res_t result = {0};
static void jpeg_decoder_isr(void);
#endif

uint32_t image_ppi = 0;
bk_err_t bk_jpeg_dec_driver_init(void)
{
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_ON);
	if(sys_drv_jpeg_dec_set(1, 1) != 0) {
		os_printf("jpeg dec sys clk config error \r\n");
		return BK_FAIL;
	}
#if (USE_JPEG_DEC_COMPLETE_CALLBACKS == 1)
	bk_int_isr_register(INT_SRC_JPEG_DEC, jpeg_decoder_isr, NULL);
#endif
	jpg_decoder_init();

	return BK_OK;
}


bk_err_t bk_jpeg_dec_driver_deinit(void)
{
	if(sys_drv_jpeg_dec_set(0, 0) != 0) {
		os_printf("jpeg dec sys clk config error \r\n");
		return BK_FAIL;
	}
	bk_int_isr_unregister(INT_SRC_JPEG_DEC);
	jpg_decoder_deinit();
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_VIDP_JPEG_DE, PM_POWER_MODULE_STATE_OFF);	
	return BK_OK;
}


bk_err_t bk_jpeg_dec_line_int_en(uint32_t line_num)
{
	jpeg_dec_auto_line_num_int_en(1, (line_num / 8));  //line num mast be multiple of 8
	return BK_OK;
}

bk_err_t bk_jpeg_dec_line_int_dis(void)
{
	jpeg_dec_auto_line_num_int_en(0, 0);  //line num mast be multiple of 8
	return BK_OK;
}

JRESULT bk_jpeg_dec_hw_start(uint32_t length, unsigned char *input_buf, unsigned char * output_buf)
{
	int ret = 0;

	ret = JpegdecInit(length, input_buf, output_buf, &image_ppi);
	if(ret != JDR_OK)
	{
		os_printf("JpegdecInit error %x \r\n", ret);
		return ret;
	}
	ret = jd_decomp();
	if(ret != JDR_OK)
	{
		os_printf("jd_decomp error %x \r\n", ret);
		return ret;
	}
	return JDR_OK;
}
bk_err_t bk_jpeg_dec_stop(void)
{
	jpeg_dec_ll_set_reg0x5_mcu_x(0);
	jpeg_dec_ll_set_reg0x6_mcu_y(0);
	jpeg_dec_ll_set_reg0x8_dec_cmd(JPEGDEC_DC_CLEAR);
	jpeg_dec_ll_set_reg0x5f_value(0x1ff);
	jpeg_dec_ll_set_reg0x3_value(0);
	jpeg_dec_ll_set_reg0x0_jpeg_dec_en(0);
	
	return BK_OK;
}


#if (USE_JPEG_DEC_COMPLETE_CALLBACKS == 1)
bk_err_t bk_jpeg_dec_isr_register(jpeg_dec_isr_type_t isr_id, jpeg_dec_isr_cb_t cb_isr)
{
	if ((isr_id) >= DEC_ISR_MAX) 
		return BK_FAIL;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_jpeg_dec_isr[isr_id] = cb_isr;
	GLOBAL_INT_RESTORE();
	return BK_OK;
}
static void jpeg_decoder_isr(void)
{
	if (jpeg_dec_ll_get_reg0x5f_dec_frame_int_clr()) {

		result.size = jpeg_dec_ll_get_reg0x5d_value();
		result.pixel_x = image_ppi >> 16;
		result.pixel_y = image_ppi & 0xFFFF;

		if (result.size)
		{
			result.size += 2;
		}

		if(jpeg_dec_ll_get_reg0x2_jpeg_dec_linen())  //enable line num en
		{
			if(jpeg_dec_ll_get_reg0x1_mcu_index() == 0) {
				jpeg_dec_ll_set_reg0x0_jpeg_dec_en(0);
				jpeg_dec_ll_set_reg0x0_jpeg_dec_en(3);
				jpeg_dec_ll_set_reg0x5f_dec_frame_int_clr(1);
				if (s_jpeg_dec_isr[DEC_END_OF_FRAME]) {
					s_jpeg_dec_isr[DEC_END_OF_FRAME](&result);
				}
			} else {
				jpeg_dec_ll_set_reg0x8_dec_cmd(JPEGDEC_START);
				jpeg_dec_ll_set_reg0x5f_dec_frame_int_clr(1);
				if (s_jpeg_dec_isr[DEC_END_OF_LINE_NUM]) {
					s_jpeg_dec_isr[DEC_END_OF_LINE_NUM](&result);
				}
			} 
		} else {
//			jpeg_dec_ll_set_reg0x0_jpeg_dec_en(3);
			jpeg_dec_ll_set_reg0x5_mcu_x(0);
			jpeg_dec_ll_set_reg0x6_mcu_y(0);
			jpeg_dec_ll_set_reg0x8_dec_cmd(JPEGDEC_DC_CLEAR);
			jpeg_dec_ll_set_reg0x5f_value(0x1ff);
			jpeg_dec_ll_set_reg0x3_value(0);
//			jpeg_dec_ll_set_reg0x5f_dec_frame_int_clr(1);
			jpeg_dec_ll_set_reg0x0_jpeg_dec_en(0);

			if (s_jpeg_dec_isr[DEC_END_OF_FRAME]) {
				s_jpeg_dec_isr[DEC_END_OF_FRAME](&result);
			}
		}
	} else {
		os_printf("int status = %x not auto int and line int \r\n", jpeg_dec_ll_get_reg0x5f_value());
	}
}



#else
bk_err_t  bk_jpeg_dec_isr_register(jpeg_dec_isr_t jpeg_dec_isr)
{
	bk_int_isr_register(INT_SRC_JPEG_DEC, jpeg_dec_isr, NULL);
	return BK_OK;
}
#endif



