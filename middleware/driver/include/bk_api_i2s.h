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
#include "bk_api_i2s_types.h"

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
bk_err_t bk_i2s_driver_init(void);

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
bk_err_t bk_i2s_driver_deinit(void);

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
	bk_err_t bk_i2s_init(i2s_gpio_group_id_t id, const i2s_config_t *config);

bk_err_t bk_i2s_deinit(void);
bk_err_t bk_i2s_get_read_ready(uint32_t *read_flag);
bk_err_t bk_i2s_get_write_ready(uint32_t *write_flag);
bk_err_t bk_i2s_enable(i2s_en_t en_value);
bk_err_t bk_i2s_int_enable(i2s_isr_id_t int_id, uint32_t value);
bk_err_t bk_i2s_set_role(i2s_role_t role);
bk_err_t bk_i2s_set_work_mode(i2s_work_mode_t work_mode);
bk_err_t bk_i2s_set_lrck_invert(i2s_lrck_invert_en_t lrckrp);
bk_err_t bk_i2s_set_sck_invert(i2s_sck_invert_en_t sck_invert);
bk_err_t bk_i2s_set_lsb_first(i2s_lsb_first_en_t lsb_first);
bk_err_t bk_i2s_set_sync_len(uint32_t sync_len);
bk_err_t bk_i2s_set_data_len(uint32_t data_len);
bk_err_t bk_i2s_set_pcm_dlen(uint32_t pcm_dlen);
bk_err_t bk_i2s_set_store_mode(i2s_lrcom_store_mode_t store_mode);
bk_err_t bk_i2s_clear_rxfifo(void);
bk_err_t bk_i2s_clear_txfifo(void);
bk_err_t bk_i2s_clear_txudf_int(void);
bk_err_t bk_i2s_clear_rxovf_int(void);
bk_err_t bk_i2s_set_txint_level(i2s_txint_level_t txint_level);
bk_err_t bk_i2s_set_rxint_level(i2s_rxint_level_t rxint_level);
bk_err_t bk_i2s_write_data(uint32_t channel_id, uint32_t *data_buf, uint32_t data_len);
bk_err_t bk_i2s_read_data(uint32_t *data_buf, uint32_t data_len);
bk_err_t bk_i2s_get_data_addr(uint32_t *i2s_data_addr);
bk_err_t bk_i2s_set_ratio(i2s_rate_t *rate);
bk_err_t bk_i2s_register_i2s_isr(i2s_isr_id_t isr_id, i2s_isr_t isr, void *param);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
