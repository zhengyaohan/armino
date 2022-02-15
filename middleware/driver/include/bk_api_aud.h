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
#include "bk_api_aud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief AUD API
 * @defgroup bk_api_aud AUD API group
 * @{
 */

/**
 * @brief     Init the AUD driver
 *
 * This API init the resoure common:
 *   - Init AUD driver control memory
 *   - Configure clock and power
 *   - Configure mic enable
 *   - Register AUD isr interrupt
 *
 * This API should be called before any other AUD APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_driver_init(void);

/**
 * @brief     Deinit the AUD driver
 *
 * This API free all resource related to AUD, power down AUD and mic.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_driver_deinit(void);

/**
 * @brief     Init the adc module of audio
 *
 * This API init the adc module:
 *  - Set adc work mode: adc/dtmf
 *  - Configure the adc/dtmf parameters
 *  - disable adc/dtmf
 *  - disable adc/dtmf interrupts
 *
 * @param
 *    - adc_work_mode: adc work mode adc/dtmf
 *    - adc_config: adc configure of adc work mode
 *    - dtmf_config: dtmf configure of dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is error
 *    - BK_ERR_NULL_PARAM: config is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_init(aud_adc_work_mode_t adc_work_mode, const aud_adc_config_t *adc_config, const aud_dtmf_config_t *dtmf_config);

/**
 * @brief     Deinit adc module
 *
 * This API deinit the adc module of audio:
 *   - Disable adc and dtmf
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_deinit(void);

/**
 * @brief     Set the sample rate in adc work mode
 *
 * @param
 *    - samp_rate: adc sample rate of adc work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_set_adc_samp_rate(aud_adc_samp_rate_t samp_rate);

/**
 * @brief     Get the adc fifo address in adc work mode
 *
 * @param
 *    - adc_fifo_addr: adc fifo address of adc work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_adc_fifo_addr(uint32_t *adc_fifo_addr);

/**
 * @brief     Get the dtmf fifo address in adc work mode
 *
 * @param
 *    - dtmf_fifo_addr: dtmf fifo address of dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_dtmf_fifo_addr(uint32_t *dtmf_fifo_addr);

/**
 * @brief     Get the dac fifo address
 *
 * @param
 *    - dac_fifo_addr: dac fifo address
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_dac_fifo_addr(uint32_t *dac_fifo_addr);

/**
 * @brief     Get the adc status information in adc work mode
 *
 * This API get the adc status of adc work mode:
 *   - Get fifo status
 *   - Get agc status
 *
 * @param
 *    - adc_status: adc fifo status and agc status
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_adc_status(aud_adc_status_t *adc_status);

/**
 * @brief     Get the dtmf status information in dtmf work mode
 *
 * This API get the adc status of dtmf work mode:
 *   - Get fifo status
 *
 * @param
 *    - dtmf_status: dtmf fifo status
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_dtmf_status(aud_dtmf_status_t *dtmf_status);

/**
 * @brief     Start ADC to DAC loop test
 *
 * This API start loop test:
 *   - Start adc to dac loop test if work mode is adc work mode
 *   - Start dtmf to dac loop test if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_start_loop_test(void);

/**
 * @brief     Stop ADC to DAC loop test
 *
 * This API start loop test:
 *   - Stop adc to dac loop test if work mode is adc work mode
 *   - Stop dtmf to dac loop test if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_stop_loop_test(void);

/**
 * @brief     Enable adc interrupt
 *
 * This API enable adc interrupt:
 *   - Enable adc interrupt if work mode is adc work mode
 *   - Enable dtmf interrupt if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_enable_adc_int(void);

/**
 * @brief     Disable adc interrupt
 *
 * This API disable adc interrupt:
 *   - Disable adc interrupt if work mode is adc work mode
 *   - Disable dtmf interrupt if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_disable_adc_int(void);

/**
 * @brief     Start adc
 *
 * This API start adc:
 *   - Enable adc if work mode is adc work mode
 *   - Enable dtmf if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_start_adc(void);

/**
 * @brief     Stop adc
 *
 * This API stop adc:
 *   - Disable adc if work mode is adc work mode
 *   - Disable dtmf if work mode is dtmf work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_stop_adc(void);

/**
 * @brief     Get adc data
 *
 * This API get adc fifo data
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_adc_fifo_data(uint32_t *adc_data);

/**
 * @brief     Get dtmf data
 *
 * This API get dtmf fifo data
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: dtmf work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_dtmf_fifo_data(uint32_t *dtmf_data);

/**
 * @brief     Init the dac module of audio
 *
 * This API init the dac module:
 *  - Configure the dac parameters
 *
 * @param
 *    - dac_config: dac configure
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: config is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dac_init(const aud_dac_config_t *dac_config);

/**
 * @brief     Deinit dac module
 *
 * This API deinit the dac module of audio:
 *   - Disable dac
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_dac_deinit(void);

/**
 * @brief     Set the dac sample rate
 *
 * @param
 *    - samp_rate: dac sample rate
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_set_dac_samp_rate(aud_dac_samp_rate_t samp_rate);

/**
 * @brief     write the sampled value to dac fifo 
 *
 * @param
 *    - pcm_value: sampled value
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_dac_write(uint32_t pcm_value);

/**
 * @brief     Set the interrupt threshold of dac fifo read
 *
 * @param
 *    - dacl_throld: the interrupt threshold of dac left channel
 *    - dacr_throld: the interrupt threshold of dac right channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_set_dac_read_threshold(uint16_t dacl_throld, uint16_t dacr_throld);

/**
 * @brief     Enable dac interrupt
 *
 * This API enable dac interrupt:
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_enable_dac_int(void);

/**
 * @brief     Disable dac interrupt
 *
 * This API disable dac interrupt:
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
 bk_err_t bk_aud_disable_dac_int(void);

/**
 * @brief     Get the dac status information
 *
 * This API get the dac status:
 *   - Get fifo status
 *
 * @param
 *    - dac_status: dac fifo status
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_get_dac_status(aud_dac_status_t *dac_status);

/**
 * @brief     Start dac
 *
 * This API start dac function.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_start_dac(void);

/**
 * @brief     Stop dac
 *
 * This API stop dac function.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_stop_dac(void);

/**
 * @brief     Register audio isr
 *
 * This API register audio isr:
 *   - Disable adc if work mode is adc work mode
 *   - Disable dtmf if work mode is dtmf work mode
 *
 * @param
 *    - isr_id: adc work mode adc/dtmf
 *    - isr: audio isr callback
 *    - param: audio isr callback parameter
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_MODE: adc work mode is NULL
 *    - BK_ERR_AUD_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_register_aud_isr(aud_isr_id_t isr_id, aud_isr_t isr, void *param);

/**
 * @brief     Control audio interrupt enable/disable of cpu
 *
 * @param
 *    - value: enable/disable audio interrupt of cpu
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_cpu_int_en(uint32_t value);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
