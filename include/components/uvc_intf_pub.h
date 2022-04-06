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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Init the uvc
 *
 * This API init psram, dma and uvc param
.*
 * param data: configure for camera and jpeg
 *
 * @attention 1. work for uvc
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_intfer_init(void);

/**
 * @brief     Denit the uvc
 *
 * This API close the modules at init
.*
 * param data: configure for camera and jpeg
 *
 * @attention 1. work for uvc
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_intfer_deinit(void);

/**
 * @brief     set uvc dma
 *
 * This API will config the dma for uvc
 *
 * @attention 1. work for uvc
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_set_dma(void);

/**
 * @brief     read uvc frame
 *
 * This API will save a frame to sdcard if sdcard_config enable
 *
 * param file_id: frame
 *
 * @attention 1. work for uvc
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_read_frame(uint8_t file_id);

/**
 * @brief     set uvc pps and fps
 *
 * This API will set uvc pps and fps
 *
 * param ppi: image resolution
 * param fps: the number frame output every second
 *
 * @attention 1. need call before uvc_set_start
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_set_ppi_fps(uint16_t ppi, uint8_t fps);

/**
 * @brief     set uvc start
 *
 * This API will set uvc start
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_set_start(void);

/**
 * @brief     set uvc stop
 *
 * This API will set uvc stop
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_set_stop(void);

/**
 * @brief     register uvc frame end isr_func
 *
 * This API will excute the isr_func if register
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t uvc_register_frame_end_isr(void *callback);

#ifdef __cplusplus
}
#endif

