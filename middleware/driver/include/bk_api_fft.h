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
//
#pragma once
#include "bk_include.h"
#include "bk_api_fft_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief FFT API
 * @defgroup bk_api_fft FFT API group
 * @{
 */


/**
 * @brief     Get the fft module working status
 *
 * @param
 *    - busy_flag: save the busy status of fft
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_fft_is_busy(uint32_t *busy_flag);

/**
 * @brief     Enable fft module to process fft/ifft function
 *
 * This API process fft/ifft function :
 *  - Set fft work mode: fft/ifft
 *  - Configure the fft/ifft parameters
 *  - start trigger
 *  - enable interrupts
 *
 * @param
 *    - fft_conf: fft/ifft parameters
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_fft_enable(fft_input_t *fft_conf);

/**
 * @brief     Enable fft module to process fir function
 *
 * This API process fir function :
 *  - Set fir work mode: signal/dual
 *  - Configure the fir parameters
 *  - start trigger
 *  - enable interrupts
 *
 * @param
 *    - fir_conf: fir parameters
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_fft_fir_single_enable(fft_fir_input_t *fir_conf);

/**
 * @brief     Init fft module driver
 *
 * This API init fft driver :
 *  - Power on clock
 *  - enable interrupts
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_fft_driver_init(void);

/**
 * @brief     Deinit fft module driver
 *
 * This API deinit fft driver :
 *  - Power down clock
 *  - disable interrupts
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_fft_driver_deinit(void);

/**
 * @brief     Read fft data
 *
 * This API read fft data after processing complete
 *

 * @param
 *    - i_output: save fft data
 *    - q_output: save fft data
 *    - size:   read size if i_output
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_fft_output_read(int16 *i_output, int16 *q_output, uint32_t size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
