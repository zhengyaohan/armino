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

#include "driver/lcd_types.h"

#ifdef __cplusplus
extern "C" {
#endif



/* @brief Overview about this API header
 *
 */

/**
 * @brief LCD API
 * @{
 */

 /**
 * @brief    This API select LCD module clk source
 *          - config lcd freq div
 *          - config video power
 *          - open lcd sys interrupt enable
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_driver_init(lcd_clk_t clk);


/**
 * @brief This API init the 8080 lcd interface
 *    - Set lcd display mode is 8080 interface
 *    - init 8080 lcd gpio 
 *    - enable 8080 display
 *    -enable 8080 end of frame interrupt
 *    - if you want enable start of frame interrupt, please use API bk_lcd_8080_int_enable
 *
 * @param
 *     - x_pixel defult by 320, user can set by any value.
 *     - y_pixel defult by 480, user can set by any value.
 *     - input_data_format
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_8080_init(uint16_t x_pixel, uint16_t y_pixel,rgb_input_data_format_t input_data_format);


/**
* @brief 8080 lcd interface reg deinit
*     - This API reset all lcd reg include power 
*     - close 8080/rgb lcd enable and display
*     - reset x pixel and y pixel zero
*     - unregister lcd isr
*
* @return
*	  - BK_OK: succeed
*	  - others: other errors.
*/
bk_err_t bk_lcd_8080_deinit(void);

/**
 * @brief This API config lcd display x size and y size
 *
 * @param
 *     - width lcd display width
 *     - height lcd display height
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_pixel_config(uint16_t x_pixel, uint16_t y_pixel);


/**
 * @brief This API config 8080 lcd interrupt

 * @param
 *     - is_sof_en enable start of frame interrupt
 *     - is_eof_en enable end of frame interrupt
 *
 * @attention 8080 end of frame int is open in API bk_lcd_8080_init
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_8080_int_enable(bool is_sof_en, bool is_eof_en);


/**
 * @brief This API start 8080 lcd transfer data to display
 *
 * @param start_transfer 
 *      - 1:data start transfer to lcd display on; 
 *      - 0:stop  transfer
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_8080_start_transfer(bool start);


/**
 * @brief     rgb lcd interface reg deinit
 *           - This API reset all lcd reg include power 
 *           - close 8080/rgb lcd enable and display
 *           - reset x pixel and y pixel zero
 *           - unregister lcd isr
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_rgb_deinit(void);

/**
 * @brief  enable rgb lcd enable
 * @param  bool en
 *       - 1: enable rgb display
 *       - 0: disable rgb display
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_rgb_display_en(bool en);


/**
* @brief This API config rgb lcd interrupt

 * @param
 *     - is_sof_en enable start of frame interrupt
 *     - is_eof_en enable end of frame interrupt
 *
 * @attention rgb end of frame int is open in API bk_lcd_rgb_init
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_rgb_int_enable(bool is_sof_en, bool is_eof_en);

#if	(USE_LCD_REGISTER_CALLBACKS == 1) 

/**
 * @brief This API register  8080/rgb lcd int isr
 * 
 * @param
 *     - int_type  include  8080/rgb  end of frame int and  8080/rgb  start of frame int
 *     - isr: isr function
 *
 * Usage example:
 *
 *       bk_lcd_isr_register(I8080_OUTPUT_EOF, lcd_i8080_isr); //register 8080 end of frame isr
 *
  * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_isr_register(lcd_int_type_t int_type, lcd_isr_t isr);
#else

/**
 * @brief This API register  8080/rgb lcd int isr
 * 
 * @param
 *     - isr: isr function
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_isr_register(lcd_isr_t lcd_isr);

/**
 * @brief This API used to get lcd int status
 * 
 * @return
 *     - status value.
 *
 * 8080 Usage example:
 *     if (bk_lcd_int_status_get()&I8080_OUTPUT_EOF == 1), present 8080 eof int triggerd
 *     if (bk_lcd_int_status_get()&RGB_OUTPUT_EOF == 1), present rgb eof int triggerd
 *
 */
uint32_t bk_lcd_int_status_get(void);

/**
 * @brief This API used to clr lcd int status
 * 
 * @param
 *     - int_type value.
 * 
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_int_status_clear(lcd_int_type_t int_type);
#endif


/**
 * @brief This API init the rgb lcd interface
 *    - Set lcd display mode is rgb interface
 *    - init rgb lcd gpio 
 *    - enable rgb display
 *    - enable rgb end of frame interrupt
 *
 * @param
 *     - lcd_device_id_t: lcd type
 *     - x_pixel
 *     - y_pixel
 *     - input_data_format:  input_data_format select rgb565 data, yuyv, yyuv or other yuv *        mode from struct rgb_input_data_format_t
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_rgb_init(lcd_device_id_t id, uint16_t x_pixel, uint16_t y_pixel, rgb_input_data_format_t input_data_format);


/**
 * @brief This API set display read mem addr
 * 
 * @param disp_base_addr lcd display base addr
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_set_display_base_addr(uint32_t disp_base_addr);

/**
 * @brief This API used send 8080 lcd init cmd
 * 
 * @param
 *     - param_count: cmd parameter number
 *     - command: command
 *     - param: the cmd parameter
 *
 * Usage example:
 *
 *    #define COMMAND_1 0xf
 *    uint32_t param_command1[2]   = {0xc3, 0x29};
 *    bk_lcd_8080_send_cmd(2, COMMAND_1, param_command1);
 *
  * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_8080_send_cmd(uint8_t param_count, uint32_t command, uint32_t *param);

/**
 * @brief This API used get lcd display base addr
 * 
  * @return lcd display addr
 */
uint32_t bk_lcd_get_display_base_addr(void);

/**
 * @brief This API used for display partical area
 * 
 * @param
 *     - partial_clum_l: 
 *     - partial_clum_r:
 *     - partial_line_l: 
 *     - partial_line_r: 
 *
 * 8080 Usage example:
 *
 *    #define PARTICAL_XS   101
 *    #define PARTICAL_XE   220
 *    #define PARTICAL_YS   101
 *    #define PARTICAL_YE   380
 *    bk_lcd_set_partical_display(EDGE_PARTICAL_XS, EDGE_PARTICAL_XE, EDGE_PARTICAL_YS, EDGE_PARTICAL_YE);
 *    bk_lcd_8080_send_cmd(2, COMMAND_1, param_command1);
 *
  * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_set_partical_display(bool en, uint16_t partial_clum_l, uint16_t partial_clum_r, uint16_t partial_line_l, uint16_t partial_line_r);


/**
 * @brief This API used for 8080 mcu lcd 0x2c/0x3c. ram write cmd send
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t bk_lcd_8080_ram_write(uint32_t cmd);


/****************************next API exampel please refer to ./complent/media/lcd demo*****************/
/**
 * @brief This API used for init lcd 
 * 
 *
 * @param lcd_config_t
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 *
 *  Usage example:
 *    lcd_config_t lcd_config;
 *    lcd_config.device = get_lcd_device_by_id(LCD_DEVICE_ST7282);
 *    lcd_config.complete_callback = lcd_complete_callback;
 *    lcd_config.fmt = VUYY_DATA;
 *    lcd_config.pixel_x = 640;
 *    lcd_config.pixel_y = 480;
 *    lcd_driver_init(&lcd_config);
 *
 */
bk_err_t lcd_driver_init(const lcd_config_t *config);

/**
 * @brief this api used to get lcd device interface
 * 
 * @param lcd_device_id_t
 * @return lcd_device_id_t
 */
const lcd_device_t *get_lcd_device_by_id(lcd_device_id_t id);

/**
 * @brief this api used to set lcd backlight
 * 
 * @param percent rang from 0~100
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t lcd_driver_set_backlight(uint8_t percent);

/**
 * @brief this api used to enable lcd display
 * 
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t lcd_driver_display_enable(void);

/**
 * @brief this api used to clear eof int and diaplay continue
 * 
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t lcd_driver_display_continue(void);

/**
 * @brief this api used to set lcd display addr
 * 
 * @param disp_base_addr sram or psram
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t lcd_driver_set_display_base_addr(uint32_t disp_base_addr);

/**
 * @brief this api used to close lcd
 *
 * @return
 *     - BK_OK: succeed
 *     - others: other errors.
 */
bk_err_t lcd_driver_deinit(void);


/**
  * @}
  */

#ifdef __cplusplus
 }
#endif





