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

#include "bk_api_jpeg_types.h"

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
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_jpeg_driver_deinit(void);

bk_err_t bk_jpeg_init(const jpeg_config_t *config);

bk_err_t bk_jpeg_deinit(void);

bk_err_t bk_jpeg_set_x_pixel(uint32_t x_pixel);

bk_err_t bk_jpeg_set_y_pixel(uint32_t y_pixel);

bk_err_t bk_jpeg_set_yuv_mode(uint32_t mode);

uint32_t bk_jpeg_get_frame_byte_number(void);

bk_err_t bk_jpeg_register_frame_start_isr(jpeg_isr_t isr, void *param);

bk_err_t bk_jpeg_register_frame_end_isr(jpeg_isr_t isr, void *param);

bk_err_t bk_jpeg_register_end_yuv_isr(jpeg_isr_t isr, void *param);

bk_err_t bk_jpeg_gpio_enable_func2(void);

#ifdef __cplusplus
}
#endif

