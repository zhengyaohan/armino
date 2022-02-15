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

#include "include.h"
#include "i2s_hal.h"
#include "i2s_cap.h"
#include "i2s_driver.h"
#include "sys_driver.h"
#include "clock_driver.h"
#include "bk_api_i2s_types.h"
#include "rtos_pub.h"
#include "mem_pub.h"
#include "bk_api_int.h"
#include "gpio_driver.h"
#include "bk_api_gpio.h"


typedef struct {
	i2s_isr_t callback;
	void *param;
} i2s_callback_t;

#define NUMBER_ROUND_UP(a,b)        ((a) / (b) + (((a) % (b)) ? 1 : 0))
//#define NUMBER_ROUND_DOWN(a,b)      ((a) / (b))

#define I2S_RETURN_ON_NOT_INIT() do {\
		if (!s_i2s_driver_is_init) {\
			return BK_ERR_I2S_NOT_INIT;\
		}\
	} while(0)

#define I2S_RETURN_ON_INVALID_ISR_ID(isr_id) do {\
		if ((isr_id) >= SOC_I2S_ISR_NUM) {\
			return BK_ERR_I2S_ISR_ID;\
		}\
	} while(0)


static bool s_i2s_driver_is_init = false;
static i2s_callback_t s_i2s_isr[SOC_I2S_ISR_NUM] = {NULL};
//static i2s_driver_t driver_i2s;
static i2s_role_t i2s_role = FFT_ROLE_MAX;

static void i2s_isr(void);
extern void delay(int num);//TODO fix me


static void i2s_init_gpio(i2s_gpio_group_id_t id)
{
	switch(id)
	{
		case I2S_GPIO_GROUP_0:
			gpio_dev_unmap(GPIO_6);
			gpio_dev_map(GPIO_6, GPIO_DEV_I2S1_CLK);
			bk_gpio_disable_output(GPIO_6);
			gpio_dev_unmap(GPIO_7);
			gpio_dev_map(GPIO_7, GPIO_DEV_I2S1_SYNC);
			bk_gpio_disable_output(GPIO_7);
			gpio_dev_unmap(GPIO_8);
			gpio_dev_map(GPIO_8, GPIO_DEV_I2S1_DIN);
			bk_gpio_disable_output(GPIO_8);
			gpio_dev_unmap(GPIO_9);
			gpio_dev_map(GPIO_9, GPIO_DEV_I2S1_DOUT);
			bk_gpio_disable_output(GPIO_9);
			gpio_dev_unmap(GPIO_28);
			gpio_dev_map(GPIO_28, GPIO_DEV_I2S1_MCLK);
			bk_gpio_disable_output(GPIO_28);
			break;

		case I2S_GPIO_GROUP_1:
			gpio_dev_unmap(GPIO_40);
			gpio_dev_map(GPIO_40, GPIO_DEV_I2S1_CLK);
			bk_gpio_disable_output(GPIO_40);
			gpio_dev_unmap(GPIO_41);
			gpio_dev_map(GPIO_41, GPIO_DEV_I2S1_SYNC);
			bk_gpio_disable_output(GPIO_41);
			gpio_dev_unmap(GPIO_42);
			gpio_dev_map(GPIO_42, GPIO_DEV_I2S1_DIN);
			bk_gpio_disable_output(GPIO_42);
			gpio_dev_unmap(GPIO_43);
			gpio_dev_map(GPIO_43, GPIO_DEV_I2S1_DOUT);
			bk_gpio_disable_output(GPIO_43);
			gpio_dev_unmap(GPIO_28);
			gpio_dev_map(GPIO_28, GPIO_DEV_I2S1_MCLK);
			bk_gpio_disable_output(GPIO_28);
			break;

		default:
			break;
	}
}

bk_err_t bk_i2s_driver_init(void)
{
	if (s_i2s_driver_is_init)
		return BK_OK;

	//power on
	//select 26M XTAL clock and enable i2s clock
	sys_drv_i2s_select_clock(0);
	sys_drv_i2s_clock_en(1);

	//i2s_disckg always on
	sys_drv_i2s_disckg_set(1);

	//enable i2s interrupt
	sys_drv_cpu0_i2s_int_en(1);

	os_memset(&s_i2s_isr, 0, sizeof(s_i2s_isr));

	//register fft isr
	i2s_int_config_t int_config_table = {INT_SRC_I2S, i2s_isr};
	bk_int_isr_register(int_config_table.int_src, int_config_table.isr, NULL);
	s_i2s_driver_is_init = true;

	return BK_OK;
}

bk_err_t bk_i2s_driver_deinit(void)
{
	//power down
	sys_drv_i2s_clock_en(0);

	//i2s_disckg not always on
	sys_drv_i2s_disckg_set(0);
	//disable i2s interrupt
	sys_drv_cpu0_i2s_int_en(0);

	i2s_int_config_t int_config_table = {INT_SRC_I2S, i2s_isr};
	bk_int_isr_unregister(int_config_table.int_src);
	s_i2s_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_i2s_init(i2s_gpio_group_id_t id, const i2s_config_t *config)
{
	I2S_RETURN_ON_NOT_INIT();
	if (!config)
		return BK_ERR_I2S_PARAM;

	i2s_init_gpio(id);
	i2s_hal_config(config);
	i2s_role = config->role;

	return BK_OK;
}

bk_err_t bk_i2s_deinit(void)
{
	I2S_RETURN_ON_NOT_INIT();

	i2s_hal_deconfig();
	i2s_role = FFT_ROLE_MAX;

	return BK_OK;
}

bk_err_t bk_i2s_get_read_ready(uint32_t *read_flag)
{
	i2s_hal_read_ready_get(read_flag);
	return BK_OK;
}

bk_err_t bk_i2s_get_write_ready(uint32_t *write_flag)
{
	i2s_hal_write_ready_get(write_flag);
	return BK_OK;
}

bk_err_t bk_i2s_enable(i2s_en_t en_value)
{
	i2s_hal_en_set(en_value);
	return BK_OK;
}

bk_err_t bk_i2s_int_enable(i2s_int_en_config_t *int_config)
{
	i2s_hal_int_set(int_config);
	return BK_OK;
}

bk_err_t bk_i2s_set_role(i2s_role_t role)
{
	i2s_hal_role_set(role);
	return BK_OK;
}

bk_err_t bk_i2s_set_work_mode(i2s_work_mode_t work_mode)
{
	i2s_hal_work_mode_set(work_mode);
	return BK_OK;
}

bk_err_t bk_i2s_set_lrck_invert(i2s_lrck_invert_en_t lrckrp)
{
	i2s_hal_lrck_invert_set(lrckrp);
	return BK_OK;
}

bk_err_t bk_i2s_set_sck_invert(i2s_sck_invert_en_t sck_invert)
{
	i2s_hal_sck_invert_set(sck_invert);
	return BK_OK;
}

bk_err_t bk_i2s_set_lsb_first(i2s_lsb_first_en_t lsb_first)
{
	i2s_hal_lsb_first_set(lsb_first);
	return BK_OK;
}

bk_err_t bk_i2s_set_sync_len(uint32_t sync_len)
{
	i2s_hal_sync_len_set(sync_len);
	return BK_OK;
}

bk_err_t bk_i2s_set_data_len(uint32_t data_len)
{
	i2s_hal_data_len_set(data_len);
	return BK_OK;
}

bk_err_t bk_i2s_set_pcm_dlen(uint32_t pcm_dlen)
{
	i2s_hal_pcm_dlen_set(pcm_dlen);
	return BK_OK;
}

bk_err_t bk_i2s_set_store_mode(i2s_lrcom_store_mode_t store_mode)
{
	i2s_hal_store_mode_set(store_mode);
	return BK_OK;
}

bk_err_t bk_i2s_clear_rxfifo(void)
{
	i2s_hal_rxfifo_clear();
	return BK_OK;
}

bk_err_t bk_i2s_clear_txfifo(void)
{
	i2s_hal_txfifo_clear();
	return BK_OK;
}

bk_err_t bk_i2s_set_txint_level(i2s_txint_level_t txint_level)
{
	i2s_hal_txint_level_set(txint_level);
	return BK_OK;
}

bk_err_t bk_i2s_set_rxint_level(i2s_rxint_level_t rxint_level)
{
	i2s_hal_rxint_level_set(rxint_level);
	return BK_OK;
}

bk_err_t bk_i2s_write_data(uint32_t channel_id, uint32_t *data_buf, uint32_t data_len)
{
	uint32_t i = 0;
	for (i=0; i<data_len; i++)
		i2s_hal_data_write(channel_id, data_buf[i]);
	return BK_OK;
}

bk_err_t bk_i2s_read_data(uint32_t *data_buf, uint32_t data_len)
{
	uint32_t i = 0;
	for (i=0; i<data_len; i++)
		i2s_hal_data_read(&data_buf[i]);
	return BK_OK;
}

bk_err_t bk_i2s_set_ratio(i2s_rate_t *p_rate)
{
	uint32_t bitratio, lrck_div, sys_clk = 0;

	if ((p_rate->freq != 8000) && (p_rate->freq != 16000) &&
		(p_rate->freq != 24000) && (p_rate->freq != 32000) && (p_rate->freq != 48000) &&
		(p_rate->freq != 11025) && (p_rate->freq != 22050) && (p_rate->freq != 44100))
		return BK_ERR_I2S_PARAM;

	/*set irck div*/
	if (p_rate->datawidth == 8)
		lrck_div = 7;
	else if (p_rate->datawidth == 16)
		lrck_div = 15;
	else if (p_rate->datawidth == 24)
		lrck_div = 23;
	else
		lrck_div = 31;

	/*set system  clock*/
	if (p_rate->freq == 8000) {
		if (p_rate->datawidth == 24) {
			sys_clk = 48384000;
			//sddev_control(DD_DEV_TYPE_SCTRL, CMD_SCTRL_AUDIO_PLL, &sys_clk);
		} else {
			sys_clk = 48128000;
			//sddev_control(DD_DEV_TYPE_SCTRL, CMD_SCTRL_AUDIO_PLL, &sys_clk);
		}
	} else if (p_rate->freq == 16000) {
		if ((p_rate->datawidth == 16) || (p_rate->datawidth == 8)) {
			sys_clk = 48128000;
			//sddev_control(DD_DEV_TYPE_SCTRL, CMD_SCTRL_AUDIO_PLL, &sys_clk);
		} else {
			sys_clk = 49152000;
			//sddev_control(DD_DEV_TYPE_SCTRL, CMD_SCTRL_AUDIO_PLL, &sys_clk);
		}
	} else if (p_rate->freq == 44100) {
		sys_clk = 50803200;
		//sddev_control(DD_DEV_TYPE_SCTRL, CMD_SCTRL_AUDIO_PLL, &sys_clk);
	} else {
		if (p_rate->datawidth == 24) {
			sys_clk = 50688000;
			//sddev_control(DD_DEV_TYPE_SCTRL, CMD_SCTRL_AUDIO_PLL, &sys_clk);
		} else {
			sys_clk = 49152000;
			//sddev_control(DD_DEV_TYPE_SCTRL, CMD_SCTRL_AUDIO_PLL, &sys_clk);
		}
	}

	/*set bit clock divd*/
	bitratio = MAX((NUMBER_ROUND_UP((sys_clk / 2), (p_rate->freq  * 2 * (lrck_div + 1))) - 1), 5);
/*
	value = value	| ((p_rate->datawidth - 1) << DATALEN_POSI)
			| (lrck_div << SMPRATIO_POSI)
			| (bitratio << BITRATIO_POSI);//this value is unused in slave mode

	REG_WRITE(PCM_CTRL, value);
*/
	i2s_hal_data_len_set(p_rate->datawidth - 1);
	i2s_hal_sample_ratio_set(lrck_div & 0x1F);
	i2s_hal_sck_ratio_set(bitratio & 0xFF);

	return BK_OK;
}


/* register i2s interrupt */
bk_err_t bk_i2s_register_i2s_isr(i2s_isr_id_t isr_id, i2s_isr_t isr, void *param)
{
	I2S_RETURN_ON_INVALID_ISR_ID(isr_id);
	uint32_t int_level = rtos_disable_int();
	s_i2s_isr[isr_id].callback = isr;
	s_i2s_isr[isr_id].param = param;
	rtos_enable_int(int_level);

	return BK_OK;
}

void i2s_isr(void)
{
	i2s_int_status_t i2s_status = {0};
	i2s_status.channel_id = I2S_CHANNEL_1;
	i2s_hal_int_status_get(&i2s_status);

	if (i2s_status.tx_udf) {
		if (s_i2s_isr[I2S_ISR_CHL1_TXUDF].callback) {
			s_i2s_isr[I2S_ISR_CHL1_TXUDF].callback(s_i2s_isr[I2S_ISR_CHL1_TXUDF].param);
		}
	}

	if (i2s_status.rx_ovf) {
		if (s_i2s_isr[I2S_ISR_CHL1_RXOVF].callback) {
			s_i2s_isr[I2S_ISR_CHL1_RXOVF].callback(s_i2s_isr[I2S_ISR_CHL1_RXOVF].param);
		}
	}

	if (i2s_status.tx_int) {
		if (s_i2s_isr[I2S_ISR_CHL1_TXINT].callback) {
			s_i2s_isr[I2S_ISR_CHL1_TXINT].callback(s_i2s_isr[I2S_ISR_CHL1_TXINT].param);
		}
	}

	if (i2s_status.rx_int) {
		if (s_i2s_isr[I2S_ISR_CHL1_RXINT].callback) {
			s_i2s_isr[I2S_ISR_CHL1_RXINT].callback(s_i2s_isr[I2S_ISR_CHL1_RXINT].param);
		}
	}

	if (SOC_I2S_CHANNEL_NUM >= 2) {
		os_memset(&i2s_status, 0, sizeof(i2s_status));
		i2s_status.channel_id = I2S_CHANNEL_2;
		i2s_hal_int_status_get(&i2s_status);
		if (i2s_status.tx_udf) {
			if (s_i2s_isr[I2S_ISR_CHL2_TXUDF].callback) {
				s_i2s_isr[I2S_ISR_CHL2_TXUDF].callback(s_i2s_isr[I2S_ISR_CHL2_TXUDF].param);
			}
		}

		if (i2s_status.rx_ovf) {
			if (s_i2s_isr[I2S_ISR_CHL2_RXOVF].callback) {
				s_i2s_isr[I2S_ISR_CHL2_RXOVF].callback(s_i2s_isr[I2S_ISR_CHL2_RXOVF].param);
			}
		}

		if (i2s_status.tx_int) {
			if (s_i2s_isr[I2S_ISR_CHL2_TXINT].callback) {
				s_i2s_isr[I2S_ISR_CHL2_TXINT].callback(s_i2s_isr[I2S_ISR_CHL2_TXINT].param);
			}
		}

		if (i2s_status.rx_int) {
			if (s_i2s_isr[I2S_ISR_CHL2_RXINT].callback) {
				s_i2s_isr[I2S_ISR_CHL2_RXINT].callback(s_i2s_isr[I2S_ISR_CHL2_RXINT].param);
			}
		}
	}

	if (SOC_I2S_CHANNEL_NUM >= 3) {
		os_memset(&i2s_status, 0, sizeof(i2s_status));
		i2s_status.channel_id = I2S_CHANNEL_3;
		i2s_hal_int_status_get(&i2s_status);
		if (i2s_status.tx_udf) {
			if (s_i2s_isr[I2S_ISR_CHL3_TXUDF].callback) {
				s_i2s_isr[I2S_ISR_CHL3_TXUDF].callback(s_i2s_isr[I2S_ISR_CHL3_TXUDF].param);
			}
		}

		if (i2s_status.rx_ovf) {
			if (s_i2s_isr[I2S_ISR_CHL3_RXOVF].callback) {
				s_i2s_isr[I2S_ISR_CHL3_RXOVF].callback(s_i2s_isr[I2S_ISR_CHL3_RXOVF].param);
			}
		}

		if (i2s_status.tx_int) {
			if (s_i2s_isr[I2S_ISR_CHL3_TXINT].callback) {
				s_i2s_isr[I2S_ISR_CHL3_TXINT].callback(s_i2s_isr[I2S_ISR_CHL3_TXINT].param);
			}
		}

		if (i2s_status.rx_int) {
			if (s_i2s_isr[I2S_ISR_CHL3_RXINT].callback) {
				s_i2s_isr[I2S_ISR_CHL3_RXINT].callback(s_i2s_isr[I2S_ISR_CHL3_RXINT].param);
			}
		}
	}

	if (SOC_I2S_CHANNEL_NUM == 4) {
		os_memset(&i2s_status, 0, sizeof(i2s_status));
		i2s_status.channel_id = I2S_CHANNEL_4;
		i2s_hal_int_status_get(&i2s_status);
		if (i2s_status.tx_udf) {
			if (s_i2s_isr[I2S_ISR_CHL4_TXUDF].callback) {
				s_i2s_isr[I2S_ISR_CHL4_TXUDF].callback(s_i2s_isr[I2S_ISR_CHL4_TXUDF].param);
			}
		}

		if (i2s_status.rx_ovf) {
			if (s_i2s_isr[I2S_ISR_CHL4_RXOVF].callback) {
				s_i2s_isr[I2S_ISR_CHL4_RXOVF].callback(s_i2s_isr[I2S_ISR_CHL4_RXOVF].param);
			}
		}

		if (i2s_status.tx_int) {
			if (s_i2s_isr[I2S_ISR_CHL4_TXINT].callback) {
				s_i2s_isr[I2S_ISR_CHL4_TXINT].callback(s_i2s_isr[I2S_ISR_CHL4_TXINT].param);
			}
		}

		if (i2s_status.rx_int) {
			if (s_i2s_isr[I2S_ISR_CHL4_RXINT].callback) {
				s_i2s_isr[I2S_ISR_CHL4_RXINT].callback(s_i2s_isr[I2S_ISR_CHL4_RXINT].param);
			}
		}
	}

}

