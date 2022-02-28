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

#include "arch_interrupt.h"
#include "bk_api_int_types.h"
#include "bk_include.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*i2s_isr_t)(void *param);


/**
 * @brief I2S defines
 * @defgroup bk_api_i2s_defs macos
 * @ingroup bk_api_i2s
 * @{
 */

#define BK_ERR_I2S_NOT_INIT			(BK_ERR_I2S_BASE - 1) /**< FFT driver not init */
#define BK_ERR_I2S_PARAM			(BK_ERR_I2S_BASE - 2) /**< FFT parameter invalid */
#define BK_ERR_I2S_ISR_ID			(BK_ERR_I2S_BASE - 3)

/**
 * @}
 */

/**
 * @brief I2S enum defines
 * @defgroup bk_api_i2s_enum I2S enums
 * @ingroup bk_api_i2s
 * @{
 */
typedef enum {
	I2S_GPIO_GROUP_0 = 0, /**< dtmf_int_en */
	I2S_GPIO_GROUP_1,   /**< adcl_int_en */
	I2S_GPIO_GROUP_MAX 	  /**< AUD isr id max */
} i2s_gpio_group_id_t;


typedef enum {
	I2S_ISR_CHL1_TXUDF = 0, /**< dtmf_int_en */
	I2S_ISR_CHL1_RXOVF,	  /**< adcl_int_en */
	I2S_ISR_CHL1_TXINT,	  /**< dacr_int_en */
	I2S_ISR_CHL1_RXINT,	  /**< dacl_int_en */
#if (SOC_I2S_CHANNEL_NUM > 1)
	I2S_ISR_CHL2_TXUDF, /**< dtmf_int_en */
	I2S_ISR_CHL2_RXOVF,   /**< adcl_int_en */
	I2S_ISR_CHL2_TXINT,   /**< dacr_int_en */
	I2S_ISR_CHL2_RXINT,   /**< dacl_int_en */
#endif

#if (SOC_I2S_CHANNEL_NUM > 2)
	I2S_ISR_CHL3_TXUDF, /**< dtmf_int_en */
	I2S_ISR_CHL3_RXOVF,   /**< adcl_int_en */
	I2S_ISR_CHL3_TXINT,   /**< dacr_int_en */
	I2S_ISR_CHL3_RXINT,   /**< dacl_int_en */
#endif

#if (SOC_I2S_CHANNEL_NUM > 3)
	I2S_ISR_CHL4_TXUDF, /**< dtmf_int_en */
	I2S_ISR_CHL4_RXOVF,   /**< adcl_int_en */
	I2S_ISR_CHL4_TXINT,   /**< dacr_int_en */
	I2S_ISR_CHL4_RXINT,   /**< dacl_int_en */
#endif

	I2S_ISR_MAX 	  /**< AUD isr id max */
} i2s_isr_id_t;

typedef enum {
	I2S_DISABLE = 0, /**< FFT fft work mode */
	I2S_ENABLE,    /**< FFT ifft work mode */
	I2S_EN_MAX,
} i2s_en_t;

typedef enum {
	I2S_ROLE_SLAVE = 0, /**< FFT normal mode */
	I2S_ROLE_MASTER,     /**< FFT bk5130 mode */
	I2S_ROLE_MAX,
} i2s_role_t;

typedef enum {
	I2S_WORK_MODE_I2S = 0, /**< IFFT disable */
	I2S_WORK_MODE_LEFTJUST,      /**< IFFT enable */
	I2S_WORK_MODE_RIGHTJUST, 	 /**< IFFT enable */
	I2S_WORK_MODE_RSVD, 	 /**< IFFT enable */
	I2S_WORK_MODE_SHORTFAMSYNC, 	 /**< IFFT enable */
	I2S_WORK_MODE_LONGFAMSYNC, 	 /**< IFFT enable */
	I2S_WORK_MODE_NORMAL2BD, 	 /**< IFFT enable */
	I2S_WORK_MODE_DELAY2BD, 	 /**< IFFT enable */
	I2S_WORK_MODE_MAX,
} i2s_work_mode_t;

typedef enum {
	I2S_LRCK_INVERT_DISABLE = 0, /**< FFT interrupt disable */
	I2S_LRCK_INVERT_ENABLE,      /**< FFT interrupt enable */
	I2S_LRCK_INVERT_MAX,
} i2s_lrck_invert_en_t;

typedef enum {
	I2S_SCK_INVERT_DISABLE = 0, /**< FFT interrupt disable */
	I2S_SCK_INVERT_ENABLE,      /**< FFT interrupt enable */
	I2S_SCK_INVERT_MAX,
} i2s_sck_invert_en_t;

typedef enum {
	I2S_LSB_FIRST_DISABLE = 0, /**< FFT gat on */
	I2S_LSB_FIRST_ENABLE,    /**< FFT gat off */
	I2S_LSB_FIRST_EN_MAX,
} i2s_lsb_first_en_t;

typedef enum {
	I2S_PARALLEL_DISABLE = 0, /**< FIR signal mode */
	I2S_PARALLEL_ENABLE,       /**< FIR dual mode */
	I2S_PARALLEL_EN_MAX,
} i2s_parallel_en_t;

typedef enum {
	I2S_LRCOM_STORE_LRLR = 0, /**< FIR interrupt disable */
	I2S_LRCOM_STORE_16R16L,      /**< FIR interrupt enable */
	I2S_LRCOM_STORE_MODE_MAX,
} i2s_lrcom_store_mode_t;

typedef enum {
	I2S_TXINT_LEVEL_1 = 0, /**< FIR disable */
	I2S_TXINT_LEVEL_8, /**< FIR disable */
	I2S_TXINT_LEVEL_16, /**< FIR disable */
	I2S_TXINT_LEVEL_24, /**< FIR disable */
	I2S_TXINT_LEVEL_MAX,
} i2s_txint_level_t;

typedef enum {
	I2S_RXINT_LEVEL_1 = 0, /**< FIR disable */
	I2S_RXINT_LEVEL_8, /**< FIR disable */
	I2S_RXINT_LEVEL_16, /**< FIR disable */
	I2S_RXINT_LEVEL_24, /**< FIR disable */
	I2S_RXINT_LEVEL_MAX,
} i2s_rxint_level_t;

typedef enum {
	I2S_INT_DISABLE = 0, /**< FIR signal mode */
	I2S_INT_ENABLE,       /**< FIR dual mode */
	I2S_INT_EN_MAX,
} i2s_int_en_t;

typedef enum {
	I2S_CHANNEL_1 = 0, /**< FIR signal mode */
	I2S_CHANNEL_2,       /**< FIR dual mode */
	I2S_CHANNEL_3, /**< FIR dual mode */
	I2S_CHANNEL_4, /**< FIR dual mode */
	I2S_CHANNEL_MAX,
} i2s_channel_id_t;

typedef enum {
	I2S_DATA_WIDTH_8 = 8,
	I2S_DATA_WIDTH_16 = 16,
	I2S_DATA_WIDTH_24 = 24,
	I2S_DATA_WIDTH_32 = 32,
} i2s_data_width_t;

typedef enum {
	I2S_SAMP_RATE_8000 = 0,
	I2S_SAMP_RATE_12000,
	I2S_SAMP_RATE_16000,
	I2S_SAMP_RATE_24000,
	I2S_SAMP_RATE_32000,
	I2S_SAMP_RATE_48000,
	I2S_SAMP_RATE_96000,
	I2S_SAMP_RATE_8018,
	I2S_SAMP_RATE_11025,
	I2S_SAMP_RATE_22050,
	I2S_SAMP_RATE_44100,
	I2S_SAMP_RATE_88200,
	I2S_SAMP_RATE_MAX
} i2s_samp_rate_t;


/**
 * @}
 */

/**
 * @brief I2S struct defines
 * @defgroup bk_api_i2s_structs structs in I2S
 * @ingroup bk_api_i2s
 * @{
 */
typedef struct {
	i2s_role_t role;
} i2s_driver_t;


typedef struct {
	icu_int_src_t int_src;
	int_group_isr_t isr;
} i2s_int_config_t;

typedef struct {
	i2s_en_t i2s_en;
	i2s_role_t role;
	i2s_work_mode_t work_mode;
	i2s_lrck_invert_en_t lrck_invert;
	i2s_sck_invert_en_t sck_invert;
	i2s_lsb_first_en_t lsb_first_en;
	uint32_t sync_length;
	uint32_t data_length;
	uint32_t pcm_dlength;
	uint32_t sample_ratio;
	uint32_t sck_ratio;
	i2s_parallel_en_t parallel_en;
	i2s_lrcom_store_mode_t store_mode;
	uint32_t sck_ratio_h4b;
	uint32_t sample_ratio_h2b;
	i2s_txint_level_t txint_level;
	i2s_rxint_level_t rxint_level;
} i2s_config_t;

typedef struct {
	i2s_channel_id_t channel_id;
	i2s_int_en_t tx_udf_en;
	i2s_int_en_t rx_ovf_en;
	i2s_int_en_t tx_int_en;
	i2s_int_en_t rx_int_en;
} i2s_int_en_config_t;

typedef struct {
	i2s_channel_id_t channel_id;
	bool tx_udf;
	bool rx_ovf;
	bool tx_int;
	bool rx_int;
} i2s_int_status_t;

typedef struct {
	i2s_data_width_t datawidth;
	i2s_samp_rate_t samp_rate;
} i2s_rate_t;

typedef struct {
	i2s_samp_rate_t samp_rate;
	i2s_data_width_t datawidth;
	uint32_t sys_clk;
	uint32_t smp_ratio;
	uint32_t bit_ratio;
} i2s_rate_table_node_t;

/**
 * @}
 */


#ifdef __cplusplus
}
#endif
