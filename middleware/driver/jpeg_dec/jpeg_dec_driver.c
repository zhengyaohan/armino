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
#include <driver/lcd_disp_types.h>
#include "jpeg_dec_macro_def.h"
#include "jpeg_dec_hal.h"
#include <driver/jpeg_dec.h>



void bk_jpegenc_off(void)
{
	jpegenc_off();
}

void bk_jpegenc_en(void)
{
	jpegenc_en();
}

void bk_jpeg_dec_sys_init(void)
{
	jpeg_hal_dec_init();
}


uint32_t bk_jpegdec_get_mcuy(void)
{
	return REG_JPEG_MCUY;
}

void bk_jpegdec_set_mcuy(uint32_t value)
{
	REG_JPEG_MCUY = value;
}

void bk_jpegdec_set_mcux(uint32_t value)
{
	REG_JPEG_MCUX = value;
}

void bk_jpegdec_set_dcuv(uint32_t value)
{
	REG_JPEG_DCUV = value;
}

void bk_jpec_dec_busy_clr(void)
{
	dec_busy2_clr;
}


void bk_jpegdec_init(JDEC* jdec, uint32_t * dec_src_addr)
{
	JpegdecInit(jdec, dec_src_addr);
}

//void bk_jpegdec_enable(bool en)
//{
//	addJPEG_DEC_Reg0x0 = en;
//}

void bk_jpegdec_close(void)
{
	REG_JPEG_MCUX= 0;;
	REG_JPEG_MCUY = 0;;
	dec_busy2_clr;
	REG_JPEG_ACC_REG0 = 0;
}


void bk_jpegdec_start(void)
{
		REG_DEC_START;
		dec_busy2_clr;
}

void bk_jpegdec_config(JDEC* jdec, uint8_t xpixel, uint32_t *dec_src_addr, uint32_t *dec_dest_addr)
{
	jd_decomp(jdec, xpixel, dec_src_addr, dec_dest_addr);
}

void bk_jpeg_dec_isr_register(int_isr_t jpeg_dec_isr)
{
	bk_int_isr_register(INT_SRC_JPEG_DEC, jpeg_dec_isr, NULL);
}

void bk_jpeg_dec_isr_unregister(void)
{
	bk_int_isr_unregister(INT_SRC_JPEG_DEC);
}



