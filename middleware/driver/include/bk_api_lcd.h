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

#include "bk_include.h"

#ifdef __cplusplus
extern "C" {
#endif



/* @brief Overview about this API header
 *
 */

/**
 * @brief LCD API
 * @defgroup bk_api_lcd LCD API group
 * @{
 */
#define bk_lcd_8080_get_tik()       lcd_hal_8080_get_tik()
#define bk_lcd_8080_verify_1ms_count(cnt) lcd_hal_8080_verify_1ms_count(cnt)
#define bk_lcd_8080_get_1ms_verify_count() lcd_hal_8080_get_1ms_verify_count()

typedef void (*int_isr_t)(void);


/**
 * @brief  rgb lcd io selected
 * @param  none
 * @return BK_OK:succeed; others: other errors.
 */
bk_err_t gpio_lcd_rgb_sel(void);

/**
 * @brief  8080 lcd io selected
 * @param  none
 * @return BK_OK:succeed; others: other errors.
 */
bk_err_t gpio_lcd_8080_sel(void);


/**
 * @brief  lcd sys init and reg init
 * @param  clk_src_sel 0:340,  1:480
 * @param  clk_div_l Frequency division : F/(1+clkdiv_disp_l+clkdiv_disp_h*2)
 * @param  clk_div_h
 * @return BK_OK:succeed; others: other errors.
 */
bk_err_t bk_lcd_init(uint8_t clk_src_sel, uint8_t clk_div_l, uint8_t clk_div_h);

/**
 * @brief  lcd reg deinit
 * @return BK_OK:succeed; others: other errors.
 */
bk_err_t bk_lcd_deinit(void);


/**
 * @brief  lcd 8080 interface need delay 131ms before send cmd
 * @param  none
 * @return BK_OK:succeed; others: other errors.
 */
void bk_lcd_8080_reset(void);

/**
 * @brief  lcd 8080 interface cmd send
 * @param  cmd bit[15:0]
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_8080_write_cmd(value)  lcd_hal_8080_write_cmd(value)

/**
 * @brief  lcd 8080 interface data send
 * @param  dat bit[15:0]
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_8080_write_data(value)  lcd_hal_8080_write_data(value)


/**
 * @brief  lcd int config
 * @param  is_sof_en 1:enable; 0:disable
 * @param  is_eof_en 1:enable; 0:disable
 */
#define bk_lcd_8080_int_enable(is_sof_en, is_eof_en) lcd_hal_8080_int_enable(is_sof_en, is_eof_en)

/**
 * @brief  bk_lcd_thrd_config
 * @param  wr_threshold_val,when data fifo beyond wr_threshold_val, will not receive dma tx data
 * @param  rd_threshold_val,
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_8080_set_dat_fifo_thrd(wr_threshold_val, rd_threshold_val) lcd_hal_8080_set_fifo_data_thrd(wr_threshold_val, rd_threshold_val)


/**
 * @brief  only used lcd 8080 interface data transfer clk adjust, need debug later,
 * @param  tik_cnt, select 0/1/2/3
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_8080_set_tik(tik_cnt)   lcd_hal_8080_set_tik(tik_cnt)

/**
 * @brief  bk_lcd_display_enable, usage reference lcd exampel
 * @param  en 1:enable; 0:disable
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_8080_display_enable(value)    lcd_hal_8080_display_enable(value)


/**
 * @brief  transfer start, only used in 8080
 * @param  start_transfer 1:data start transfer to lcd display on; 0:stop
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_8080_start_transfer(start_transfer)    lcd_hal_8080_start_transfer(start_transfer)

/**
 * @brief  pixel_config
 * @param  x_pixel the lcd support pixel
 * @param  y_pixel  the lcd support pixel
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_pixel_config(x_pixel, y_pixel) lcd_hal_pixel_config(x_pixel, y_pixel)

/**
 * @brief  enable rgb interface data discontinue transfer, used in rgb clk beyond 20Mhz clk 
 * @param  en 1:enable; 0:disable
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_disconti_mode(en)    lcd_hal_disconti_mode(en)

/* @brief config fifo mode, used only in 8080
 * @paramfifo_mode  0 : when bk_lcd_disp_enable(0), invalid write fifo,
                    1: valid whenever time
 */
#define bk_lcd_8080_fifo_mode(mode)  lcd_hal_8080_fifo_mode(mode)

/**
 * @brief  lcd int isr register
 * @param  lcd isr
 * @return BK_OK:succeed; others: other errors.
 */
bk_err_t bk_lcd_isr_register(int_isr_t lcd_isr);

bk_err_t bk_lcd_isr_unregister(void);

/**
 * @brief  bk_lcd_int_status_get
 * @param  none
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_int_status_get()                    lcd_hal_int_status_get()

/**
 * @brief  bk_lcd_int_status_clear
 * @param  int_type
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_int_status_clear(int_type)     lcd_hal_int_status_clear(int_type)
#define bk_lcd_8080_eof_int_clear()  lcd_hal_eof_int_status_clear()
#define bk_lcd_8080_sof_int_clear()  lcd_hal_sof_int_status_clear()

#define bk_lcd_rgb_eof_int_clear()  lcd_hal_rgb_eof_int_status_clear()
#define bk_lcd_rgb_sof_int_clear()  lcd_hal_rgb_sof_int_status_clear()

/**
 * @brief  bk_lcd_clk_div， usedin rgb interface
 * @param  clk_div rang is 0~0xf
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_rgb_clk_div(clk_div)  lcd_hal_set_rgb_clk_div(clk_div)

/**
 * @brief  config rgb data receive edge
 * @param  edge posedge/negedge
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_rgb_dclk_rev_edge(edge)  lcd_hal_set_rgb_clk_rev_edge(edge)

/**
 * @brief  config rgb interface hsync and vsync
 * @param  hsync_back_porch
 * @param  hsync_front_porch
 * @param  vsync_back_porch
 * @param  vsync_front_porch
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_sync_config(hsync_back_porch, hsync_front_porch, vsync_back_porch, vsync_front_porch) \
            lcd_hal_rgb_sync_config(hsync_back_porch, hsync_front_porch, vsync_back_porch, vsync_front_porch)

/**
 * @brief  set rgb format
 * @param  data_format  0:rgb; 1:yuv
 * @return BK_OK:succeed; others: other errors.
 */
#define bk_lcd_rgb_data_format(data_format)  lcd_hal_rgb_yuv_sel(data_format) 
#define bk_lcd_rgb_display_sel() lcd_hal_rgb_display_sel()

#define bk_lcd_rgb_fifo_thrd_set(wr_threshold_val, rd_threshold_val) lcd_hal_rgb_set_thrd(wr_threshold_val, rd_threshold_val)
#define bk_lcd_rgb_display_en(en) lcd_hal_rgb_display_en(en)

#define bk_lcd_rgb_int_enable(is_sof_en, is_eof_en) lcd_hal_rgb_int_enable(is_sof_en, is_eof_en)


#define bk_lcd_fifo_clr()   lcd_hal_mem_clr()
#define bk_lcd_rgb_disp_sel()    lcd_hal_rgb_display_sel()

void bk_lcd_debug(void);


/**
  * @}
  */

#ifdef __cplusplus
 }
#endif





