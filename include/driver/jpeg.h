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

#include <driver/jpeg_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Init the jpeg driver
 *
 * This API init the resoure common:
 *   - Init jpeg driver control memory
 *
 * @attention 1. This API should be called before any other jpeg APIs.
 * @attention 2. This API will power up video when soc is bk7256XX 
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_driver_init(void);

/**
 * @brief     Deinit the jpeg driver
 *
 * This API free all resource related to jpeg and disable jpeg.
 *
 * @attention 1. This API will power down video when soc is bk7256XX 
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_driver_deinit(void);

/**
 * @brief     Init the jpeg
 *
 * This API init the jpeg
 *  - Configure and Power up the jpeg clock
 *  - enable the JPEG ISR
 *  - Map the jpeg to dedicated GPIO port(MCLK and PCLK)
 *  - Set the jpeg parameters
 *  - set and start the dma function
 *  - Start the jpeg_encode
 *
 * @param config jpeg parameter settings
 *
 * @attention 1. only work for imaging transfer
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: jpeg config paramter is NULL
 *    - BK_ERR_JPEG_NOT_INIT: jpeg driver not init
 *    - others: other errors.
 */
bk_err_t bk_jpeg_init(const jpeg_config_t *config);

/**
 * @brief     Deinit the jpeg
 *
 * This API stop jpeg_encode, dma and close jpeg_gpio, power off jpeg_model at last.
 *
 * @attention 1. only work for imaging transfer
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_deinit(void);

/**
 * @brief     Init the jpeg
 *
 * This API init the jpeg
 *  - Configure and Power up the jpeg clock
 *  - enable the JPEG ISR
 *  - Map the jpeg to dedicated GPIO port(MCLK and PCLK)
 *  - Set the jpeg parameters
 *  - Start the jpeg_encode
 *
 * @param config jpeg parameter settings
 *
 * @attention 1. not init dma function compare with bk_jpeg_init, need user init dma

 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: jpeg config paramter is NULL
 *    - BK_ERR_JPEG_NOT_INIT: jpeg driver not init
 *    - others: other errors.
 */
bk_err_t bk_jpeg_cli_init(const jpeg_config_t *config);

/**
 * @brief     Deinit the jpeg
 *
 * This API stop jpeg_encode, close jpeg_gpio, and power off jpeg_model at last.
 *
 * @attention 1. not stop dma compare with bk_jpeg_deinit
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_cli_deinit(void);

/**
 * @brief     set jpeg width
 *
 * This API will set jpeg_encode image width value
 *
 * @param x_pixel: jpeg_encode image width
 *
 * @attention 1. This API should used after bk_jpeg_init or bk_jpeg_cli_init  and jpeg_encode not enable 
 * if you want to modefy pps
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_set_x_pixel(uint32_t x_pixel);

/**
 * @brief     set jpeg height
 *
 * This API will set jpeg_encode image height value
 *
 * @param y_pixel: jpeg_encode image height
 *
 * @attention 1. This API should used after bk_jpeg_init or bk_jpeg_cli_init  and jpeg_encode not enable 
 * if you want to modefy pps
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_set_y_pixel(uint32_t y_pixel);

/**
 * @brief     set jpeg yuv mode
 *
 * This API will set jpeg work mode
 *
 * @param mode: 0/1:jpeg_mode/yuv_mode
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_set_yuv_mode(uint32_t mode);

/**
 * @brief     set jpeg work status
 *
 * This API will set jpeg work enable/disable
 *
 * @param enable: 0/1:jpeg work disable/enable
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_set_work_status(uint8_t enable);

/**
 * @brief     get a frame byte size after jpeg
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
uint32_t bk_jpeg_get_frame_byte_number(void);

/**
 * @brief     register frame start isr
 *
 * This API will register start isr_func, need user defined
 *
 * @param isr: isr_func
 * @param param: other value(default set NULL)
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_register_frame_start_isr(jpeg_isr_t isr, void *param);

/**
 * @brief     register frame end isr
 *
 * This API will register start isr_func, need user defined
 *
 * @param isr: isr_func
 * @param param: other value(default set NULL)
 *
.* @attention 1. This API only effect when work jpeg_mode
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_register_frame_end_isr(jpeg_isr_t isr, void *param);

/**
 * @brief     register frame end isr
 *
 * This API will register start isr_func, need user defined
 *
 * @param isr: isr_func
 * @param param: other value(default set NULL)
 *
.* @attention 1. This API only effect when work yuv_mode
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_register_end_yuv_isr(jpeg_isr_t isr, void *param);

/**
 * @brief     enable jpeg gpio
 *
 * This API will enable jpeg data GPIO(d0-d7)
 *
 *
.* @attention 1. in jpeg init function only enable jpeg MCLK and PCLK GPIO
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_gpio_enable_func2(void);

#ifdef __cplusplus
}
#endif

