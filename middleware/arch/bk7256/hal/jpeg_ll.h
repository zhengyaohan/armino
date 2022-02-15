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

#include "gpio_types.h"
#include "jpeg_hw.h"
#include "jpeg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JPEG_LL_REG_BASE(_jpeg_unit_id)    (JPEG_R_BASE)

#define JPEG_GPIO_PIN_NUMBER    12
#define JPEG_GPIO_MAP \
{\
	{GPIO_27, GPIO_DEV_JPEG_MCLK},\
	{GPIO_29, GPIO_DEV_JPEG_PCLK},\
	{GPIO_30, GPIO_DEV_JPEG_HSYNC},\
	{GPIO_31, GPIO_DEV_JPEG_VSYNC},\
	{GPIO_32, GPIO_DEV_JPEG_PXDATA0},\
	{GPIO_33, GPIO_DEV_JPEG_PXDATA1},\
	{GPIO_34, GPIO_DEV_JPEG_PXDATA2},\
	{GPIO_35, GPIO_DEV_JPEG_PXDATA3},\
	{GPIO_36, GPIO_DEV_JPEG_PXDATA4},\
	{GPIO_37, GPIO_DEV_JPEG_PXDATA5},\
	{GPIO_38, GPIO_DEV_JPEG_PXDATA6},\
	{GPIO_39, GPIO_DEV_JPEG_PXDATA7},\
}

void jpeg_ll_init_quant_table(jpeg_hw_t *hw);

void jpeg_ll_set_x_pixel_value(jpeg_hw_t * hw, uint32_t x_pixel);

void jpeg_ll_set_y_pixel_value(jpeg_hw_t * hw, uint32_t y_pixel);
static inline void jpeg_ll_reset_config_to_default(jpeg_hw_t *hw)
{
	/* 1.reset REG_0x0
	 * 2.clear int_status(REG_0x6)
	 */
	hw->int_en.v = 0;
	REG_WRITE(JPEG_R_INT_STATUS, REG_READ(JPEG_R_INT_STATUS));
}

static inline void jpeg_ll_init(jpeg_hw_t *hw)
{
	jpeg_ll_reset_config_to_default(hw);
}

static inline void jpeg_ll_enable(jpeg_hw_t *hw)
{
	hw->cfg.jpeg_enc_en = 1;
}

static inline void jpeg_ll_disable(jpeg_hw_t *hw)
{
	hw->cfg.jpeg_enc_en = 0;
}

static inline void jpeg_ll_set_x_pixel(jpeg_hw_t *hw, uint32_t x_pixel)
{
	hw->cfg.x_pixel = x_pixel & JPEG_F_X_PIXEL_M;
}

static inline void jpeg_ll_set_y_pixel(jpeg_hw_t *hw, uint32_t y_pixel)
{
	hw->cfg.y_pixel = y_pixel & JPEG_F_Y_PIXEL_M;
}

static inline void jpeg_ll_set_yuv_mode(jpeg_hw_t *hw, uint32_t mode)
{
	if (mode == 1)
		hw->cfg.yuvbuff_mode = 1;
	else
		hw->cfg.yuvbuff_mode = 0;
}

static inline void jpeg_ll_enable_start_frame_int(jpeg_hw_t *hw)
{
	hw->int_en.int_en |= BIT(0);
}

static inline void jpeg_ll_disable_start_frame_int(jpeg_hw_t *hw)
{
	hw->int_en.int_en &= ~BIT(0);
}

static inline void jpeg_ll_enable_end_frame_int(jpeg_hw_t *hw)
{
	hw->int_en.int_en |= BIT(1);
}

static inline void jpeg_ll_disable_end_frame_int(jpeg_hw_t *hw)
{
	hw->int_en.int_en &= ~BIT(1);
}

/* REG_0x00:reg0->mclk_div:0x0[5:4],MCLK div  00/11:  24M;  01:16M; 10:12M,0x0,R/W*/
static inline uint32_t jpeg_ll_get_reg0_mclk_div(jpeg_hw_t *hw)
{
    return hw->int_en.mclk_div;
}

static inline void jpeg_ll_set_reg0_mclk_div(jpeg_hw_t *hw, uint32_t value)
{
    hw->int_en.mclk_div = value;
}

static inline void jpeg_ll_enable_enc_size(jpeg_hw_t *hw)
{
	hw->cfg.jpeg_enc_size = 1;
}

static inline void jpeg_ll_disable_enc_size(jpeg_hw_t *hw)
{
	hw->cfg.jpeg_enc_size = 0;
}

static inline void jpeg_ll_enable_video_byte_reverse(jpeg_hw_t *hw)
{
	hw->cfg.video_byte_reverse = 1;
}

static inline void jpeg_ll_disable_video_byte_reverse(jpeg_hw_t *hw)
{
	hw->cfg.video_byte_reverse = 0;
}

static inline void jpeg_ll_set_target_high_byte(jpeg_hw_t *hw, uint32_t high_byte)
{
	hw->target_byte_h = high_byte;
}

static inline uint32_t jpeg_ll_get_target_high_byte(jpeg_hw_t *hw)
{
	return hw->target_byte_h;
}

static inline void jpeg_ll_set_target_low_byte(jpeg_hw_t *hw, uint32_t low_byte)
{
	hw->target_byte_l = low_byte;
}

static inline uint32_t jpeg_ll_get_target_low_byte(jpeg_hw_t *hw)
{
	return hw->target_byte_l;
}

static inline void jpeg_ll_set_bitrate_step(jpeg_hw_t *hw, uint32_t step)
{
	hw->cfg.bitrate_step = step;
}

static inline void jpeg_ll_set_default_bitrate_step(jpeg_hw_t *hw)
{
	hw->cfg.bitrate_step = 3;
}

static inline uint32_t jpeg_ll_get_frame_byte_number(jpeg_hw_t *hw)
{
	return hw->byte_count_pfrm;
}

static inline uint32_t jpeg_ll_get_interrupt_status(jpeg_hw_t *hw)
{
	return hw->int_status.int_status;
}

static inline void jpeg_ll_clear_interrupt_status(jpeg_hw_t *hw, uint32_t int_status)
{
	REG_WRITE(JPEG_R_INT_STATUS, int_status);
}

static inline bool jpeg_ll_is_frame_start_int_triggered(jpeg_hw_t *hw, uint32_t int_status)
{
	return int_status & BIT(0);
}

static inline bool jpeg_ll_is_frame_end_int_triggered(jpeg_hw_t *hw, uint32_t int_status)
{
	return int_status & BIT(1);
}

static inline void jpeg_ll_set_em_base_addr(jpeg_hw_t *hw, uint32_t value)
{
	hw->em_base_addr.em_base_addr = ((value >> 16) | (0x20 << 16));
}

#ifdef __cplusplus
}
#endif

