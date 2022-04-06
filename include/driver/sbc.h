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
#include <common/bk_include.h>
#include <driver/sbc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief SBC API
 * @defgroup bk_api_sbc AUD API group
 * @{
 */

bk_err_t bk_sbc_decoder_frame_decode(SbcDecoderContext* sbc, const uint8_t *data, uint32_t length);

bk_err_t bk_sbc_decoder_bit_allocation(SbcCommonContext *sbc);

bk_err_t bk_sbc_decoder_init(SbcDecoderContext *sbc);

bk_err_t bk_sbc_decoder_deinit(void);

bk_err_t bk_sbc_decoder_mem_init(void);

bk_err_t bk_sbc_decoder_sbc_enable(void);

bk_err_t bk_sbc_decoder_get_sbc_enable_status(void);

bk_err_t bk_sbc_decoder_interrupt_enable(bool enable);

bk_err_t bk_sbc_decoder_support_msbc(bool enable);

bk_err_t bk_sbc_decoder_get_sbc_ctrl_value(void);

bk_err_t bk_sbc_decoder_clear_interrupt_status(void);

bk_err_t bk_sbc_decoder_get_interrupt_status(void);

bk_err_t bk_sbc_decoder_get_idle_status(void);

bk_err_t bk_sbc_decoder_set_res_bytel_value(uint32_t resl_value);

bk_err_t bk_sbc_decoder_set_res_bytem_value(uint32_t resm_value);

bk_err_t bk_sbc_decoder_set_res_byteh_value(uint32_t resh_value);

bk_err_t bk_sbc_decoder_set_sbc_bit_num(uint32_t bit_num);

bk_err_t bk_sbc_decoder_set_scale_factor(uint32_t sf);

bk_err_t bk_sbc_decoder_set_sbc_level(uint32_t level);

bk_err_t bk_sbc_decoder_set_sbc_res_bit(uint32_t res_bit);

bk_err_t bk_sbc_decoder_start_decode(void);

bk_err_t bk_sbc_decoder_get_decode_enable_value(void);

bk_err_t bk_sbc_decoder_get_pcm_data(void);

bk_err_t bk_sbc_decoder_get_mem0_addr(uint32_t *mem0_addr);

bk_err_t bk_sbc_decoder_get_mem1_addr(uint32_t *mem1_addr);

bk_err_t bk_sbc_decoder_mem0_write(uint32_t *data0);

bk_err_t bk_sbc_decoder_mem1_write(uint32_t *data1);

bk_err_t bk_sbc_decoder_register_sbc_isr(sbc_decoder_isr_t isr, void *param);

/**
 * @}
 */
 
#ifdef __cplusplus
 }
#endif


