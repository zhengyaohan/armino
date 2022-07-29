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

#include <driver/jpeg_enc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief JPEGENC APIs Version 1.0
 * @{
 */


/**
 * @brief     Init the jpeg encode driver
 *
 * This API init the resoure common:
 *   - Init jpegenc driver control memory
 *   - register jpegenc isr
 *
 * @attention 1. This API should be called before any other jpeg APIs.
 * @attention 2. This API will power up video when soc is bk7256XX 
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_driver_init(void);

/**
 * @brief     Deinit the jpegenc driver
 *
 * This API free all resource related to jpegenc and disable jpegenc.
 *
 * @attention 1. This API will power down video when soc is bk7256XX 
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_driver_deinit(void);

/**
 * @brief     Init the jpeg encode
 *
 * This API init the jpegenc
 *  - Configure clock and Power up the jpegenc
 *  - Enable Jpegenc interrupt
 *  - Map the jpegenc to dedicated GPIO port(MCLK and PCLK)
 *  - Set the jpegenc parameters
 *  - set and start the dma function
 *  - Enable jpeg encode module
 *
 * Usage example:
 *
 *      jpeg_config_t init_config = {
 *                          .rx_buf = xxx, // the address point to mem for receiving jpeg data
 *                          .yuv_mode = 0,
 *                          .sys_clk_div = 3, // jpeg_clk = 480 / (1+3) =120MHz
 *                          .mclk_div = 0, // MCLK=jpeg_clk/4 = 30MHz
 *                          .rx_buf_len = xxx, // the length of receive jpeg data
 *                          .node_len = xxx, // dma transfer length
 *                          .x_pixel = 80, // jpeg image resolution of width: 80 * 8 = 640
 *                          .y_pixel = 60, // jpeg image resolution of height: 60 * 8 = 480
 *                          .dma_rx_finish_handler = cb, // dma transfer finish callback
 *                      };
 *      BK_LOG_ON_ERR(bk_jpeg_init(&init_config);
 *
 *
 * @param config jpeg encode parameter settings
 *
 * @attention 1. only work for imaging transfer
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: jpegenc config paramter is NULL
 *    - BK_ERR_JPEG_NOT_INIT: jpegenc driver not init
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_init(const jpeg_config_t *config);

/**
 * @brief     Deinit the jpeg
 *
 * This API stop jpeg encode, dma and close jpeg gpio, power off jpeg module at last.
 *
 * @attention 1. only work for imaging transfer
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_deinit(void);

/**
 * @brief     Init the jpeg when use dvp
 *
 * This API init the jpeg
 *  - Configure clock and Power up the jpegenc clock
 *  - enable the JPEG encode interrupt
 *  - Map the jpegenc to dedicated GPIO port(MCLK and PCLK)
 *  - Set the jpegenc parameters
 *  - Enable Jpeg encode module
 *
 * @param config jpeg parameter settings
 *
 * @attention 1. not init dma function compare with bk_jpeg_init, need user init dma

 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: jpegenc config paramter is NULL
 *    - BK_ERR_JPEG_NOT_INIT: jpegenc driver not init
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_dvp_init(const jpeg_config_t *config);

/**
 * @brief     Deinit the jpeg when use dvp
 *
 * This API stop jpeg encode, close jpeg gpio, and power off jpeg module at last.
 *
 * @attention 1. not stop dma compare with bk_jpeg_deinit
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_dvp_deinit(void);

/**
 * @brief     set jpegenc enable
 *
 * This API will set jpegenc work enable/disable.
 *
 * @param enable: 0/1:jpegenc work disable/enable
 *
 * @attention 1: if work in jpegenc mode, this api can enable/disable jpegenc mode
 * @attention 2: if work in yuv mode, this api only can disable yuv_mode. The bk_jpeg_set_yuv_mode api also can set yuv mode enable/disable.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_set_enable(uint8_t enable);

/**
 * @brief     yuv format select
 *
 * This API will set yuv output format.
 *
 * @param value: 0:yuyv, 1:uyvy, 2:yyuv, 3:uvyy
 *
 * @attention 1: this function only used when jpeg at yuv mode
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_yuv_fmt_sel(uint32_t value);


/**
 * @brief     get a frame size output from jpegenc module, uint byte
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
uint32_t bk_jpeg_enc_get_frame_size(void);

/**
 * @brief     register frame end isr
 *
 * This API will register jpeg encode isr function, need user defined.The isr function will execute when the isr be triggered.
 *
 * @param type_id: the type of the isr
 *    - END_OF_YUV: (eoy)when jpeg encode work in yuv mode, once a complete frame have been transfer by jpeg encode module, the isr will trigger.
 *    - HEAD_OUTPUT: jpeg encode output head the isrt will be triggered, work in jpeg encode mode.
 *    - START_OFFRAME: (sof) when jpeg encode module start reveice a frame the isr will be triggered.
 *    - END_OF_FRAME: (eof) when jpeg encode module receive a complete frame the isr will be triggered, this only effect in jpeg encode mode.
 *    - VSYNC_NEGEDGE: (vng) when jpeg encode module receive a vsync negedge the isr will be triggered.
 * @param isr: isr_func
 * @param param: other value(default set NULL)
 *
.* @attention 1. This API only effect when work yuv mode
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_register_isr(jpeg_isr_type_t type_id, jpeg_isr_t isr, void *param);

/**
 * @brief     enable jpeg gpio
 *
 * This API will enable jpeg data GPIO(d0-d7), hsync and vsync
 *
 *
.* @attention 1. in jpegenc init function only enable jpeg MCLK and PCLK GPIO
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_dvp_gpio_enable(void);

/**
 * @brief     get jpeg enc fifo addr
 *
 * This API will get jpeg encode fifo addr in param
 *
 * param fifo_addr: fifo addr
 *
 * @attention 1. only work in jpeg encode mode is effective
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_get_fifo_addr(uint32_t *fifo_addr);

/**
 * @brief     enable jpeg enc int
 *
 * This API will enable jpeg different int type
 *
 * param type: different bit replace different int
 *
 * @attention 1. only work when jpeg encode driver have been called
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_enable_int(uint32_t type);

/**
 * @brief     disable jpeg enc int
 *
 * This API will disable jpeg different int type
 *
 * param type: different bit replace different int
 *
 * @attention 1. only work when jpeg encode driver have been called
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_disable_int(uint32_t type);

/**
 * @brief     set mclk output
 *
 * This API will use for camera config
 *
 * param cksel: 0-3:0:DCO/1:APLL/2:320M/3:480M,use 2 or 3
 * param ckdiv: 0-15, div = 1/(ckdiv+1)
 *
 * @attenation: this api only used for bk7256xx_mp chip, and when use this api, this api 'bk_jpeg_enc_mclk_enable' will not useful
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_set_auxs(uint32_t cksel, uint32_t ckdiv);

/**
 * @brief     set jpeg module mclk enable
 *
 * This API will set jpeg mclk output for sensor
 *
 * attenation: this api must call after bk_jpeg_enc_driver_init, and output 24MHz.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_mclk_enable(void);

/**
 * @brief     partial display init
 *
 * This API will use for partial display configure of jpeg encode
 *
 * param offset_config: x_pixel left/right offset and y_pixel left/right offset configure
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_partial_display_init(const jpeg_partial_offset_config_t *offset_config);

/**
 * @brief     partial display deinit
 *
 * This API will use for partial display deinit
 *
 * param offset_config: x_pixel left/right offset and y_pixel left/right offset configure
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_enc_partial_display_deinit(const jpeg_partial_offset_config_t *offset_config);
/**
 * @}
 */

/**
 * @brief     jpeg em base set
 *
 * This API will use for jpeg em base address, 64KB memory align.
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_set_em_base_addr(uint8_t *address);


#ifdef __cplusplus
}
#endif

