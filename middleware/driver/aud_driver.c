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
#include "aud_hal.h"
#include "aud_cap.h"
#include "aud_driver.h"
#include "sys_driver.h"
#include "clock_driver.h"
#include "bk_api_aud_types.h"
#include "rtos_pub.h"
#include "mem_pub.h"
#include "bk_api_int.h"
#include "bk_api_aud.h"


typedef struct {
	aud_isr_t callback;
	void *param;
} aud_callback_t;


#define AUD_RETURN_ON_NOT_INIT() do {\
		if (!s_aud_driver_is_init) {\
			return BK_ERR_AUD_NOT_INIT;\
		}\
	} while(0)

#define AUD_RETURN_ON_INVALID_ISR_ID(isr_id) do {\
		if ((isr_id) >= SOC_AUD_ISR_NUM) {\
			return BK_ERR_AUD_ISR_ID;\
		}\
	} while(0)

static bool s_aud_driver_is_init = false;
static aud_adc_work_mode_t adc_mode = AUD_ADC_WORK_MODE_NULL;
static aud_callback_t s_aud_isr[SOC_AUD_ISR_NUM] = {NULL};

static void aud_isr(void);

extern void delay(int num);//TODO fix me


bk_err_t bk_aud_adc_init(aud_adc_work_mode_t adc_work_mode, const aud_adc_config_t *adc_config, const aud_dtmf_config_t *dtmf_config)
{
	AUD_RETURN_ON_NOT_INIT();
	if (adc_work_mode >= AUD_ADC_WORK_MODE_MAX)
		return BK_ERR_AUD_ADC_MODE;

	adc_mode = adc_work_mode;

	switch (adc_work_mode){
		case AUD_ADC_WORK_MODE_NULL:
			break;
		case AUD_ADC_WORK_MODE_ADC:
			BK_RETURN_ON_NULL(adc_config);
			sys_drv_analog_reg12_set(0x81B0E0E0);

			//enable mic1 and mic2
			sys_drv_aud_mic1_en(1);
			sys_drv_aud_mic2_en(1);

			//reset mic after configuring parameters
			sys_drv_aud_mic_rst_set(1);
			delay(10);
			sys_drv_aud_mic_rst_set(0);

			AUD_LOGI("configure mic and adc\r\n");
			aud_hal_adc_config(adc_config);
			break;
		case AUD_ADC_WORK_MODE_DTMF:
			BK_RETURN_ON_NULL(dtmf_config);
			aud_hal_dtmf_config(dtmf_config);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t bk_aud_adc_deinit(void)
{
	adc_mode = AUD_ADC_WORK_MODE_NULL;
	aud_adc_config_t adc_config;

	adc_config.samp_rate = AUD_ADC_SAMP_RATE_8K;
	adc_config.adc_enable = AUD_ADC_DISABLE;
	adc_config.line_enable = AUD_ADC_LINE_DISABLE;
	adc_config.dtmf_enable = AUD_DTMF_DISABLE;
	adc_config.adc_hpf2_coef_B2 = 0;
	adc_config.adc_hpf2_bypass_enable = AUD_ADC_HPF_BYPASS_DISABLE;
	adc_config.adc_hpf1_bypass_enable = AUD_ADC_HPF_BYPASS_DISABLE;
	adc_config.adc_set_gain = 0x0;
	adc_config.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
	adc_config.adc_hpf2_coef_B0 = 0;
	adc_config.adc_hpf2_coef_B1 = 0;
	adc_config.adc_hpf2_coef_A0 = 0;
	adc_config.adc_hpf2_coef_A1 = 0;
	adc_config.dtmf_wr_threshold = 0;
	adc_config.adcl_wr_threshold = 0;
	adc_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
	adc_config.adcl_int_enable = AUD_ADCL_INT_DISABLE;
	adc_config.loop_adc2dac = AUD_LOOP_ADC2DAC_DISABLE;
	adc_config.agc_noise_thrd = 0;
	adc_config.agc_noise_high = 0;
	adc_config.agc_noise_low = 0;
	adc_config.agc_noise_min = 0;
	adc_config.agc_noise_tout = 0;
	adc_config.agc_high_dur = 0;
	adc_config.agc_low_dur = 0;
	adc_config.agc_min = 0;
	adc_config.agc_max = 0;
	adc_config.agc_ng_method = AUD_AGC_NG_METHOD_MUTE;
	adc_config.agc_ng_enable = AUD_AGC_NG_DISABLE;
	adc_config.agc_decay_time = AUD_AGC_DECAY_TIME_128;
	adc_config.agc_attack_time = AUD_AGC_ATTACK_TIME_8;
	adc_config.agc_high_thrd = 0;
	adc_config.agc_low_thrd = 0;
	adc_config.agc_iir_coef = AUD_AGC_IIR_COEF_1_32;
	adc_config.agc_enable = AUD_AGC_DISABLE;
	adc_config.manual_pga_value = 0;
	adc_config.manual_pga_enable = AUD_GAC_MANUAL_PGA_DISABLE;
	adc_config.adc_fracmod_manual = AUD_ADC_TRACMOD_MANUAL_DISABLE;
	adc_config.adc_fracmod = 0;

	aud_hal_adc_config(&adc_config);

	return BK_OK;
}

//TODO we should remove aud_init finally
//Implement aud_init/aud_exit to make it compitable with existing driver model
bk_err_t bk_aud_driver_init(void)
{
	if (s_aud_driver_is_init)
		return BK_OK;
	//power on
	//select 26M XTAL clock and enable audio clock
	sys_drv_aud_select_clock(0);
	sys_drv_aud_clock_en(1);
	//enable audpll en
	sys_drv_aud_audpll_en(1);

	// config analog register
	sys_drv_analog_reg12_set(0x8610E0E0);
	sys_drv_analog_reg13_set(0x0F808400);   
	sys_drv_analog_reg14_set(0x40038002);    //gain :15
	sys_drv_analog_reg15_set(0x40038002);
	sys_drv_analog_reg16_set(0x89C02401);
	sys_drv_analog_reg17_set(0x80100000);

	//enable audvdd 1.0v and 1.5v
	sys_drv_aud_vdd1v_en(1);
	sys_drv_aud_vdd1v5_en(1);

	os_memset(&s_aud_isr, 0, sizeof(s_aud_isr));

	aud_int_config_t int_config_table = {INT_SRC_AUDIO, aud_isr};
	bk_int_isr_register(int_config_table.int_src, int_config_table.isr, NULL);
	//enable audio interrupt
	sys_drv_aud_int_en(1);

	s_aud_driver_is_init = true;
	adc_mode = AUD_ADC_WORK_MODE_NULL;
	return BK_OK;
}

bk_err_t bk_aud_driver_deinit(void)
{
	if (!s_aud_driver_is_init) {
		return BK_OK;
	}

	//power down
	sys_drv_aud_clock_en(0);

	sys_drv_aud_audpll_en(0);

	//disable audvdd 1.0v and 1.5v
	sys_drv_aud_vdd1v_en(0);
	sys_drv_aud_vdd1v5_en(0);

	// config analog register
	sys_drv_analog_reg12_set(0x0);
	sys_drv_analog_reg13_set(0x0);
	sys_drv_analog_reg14_set(0x0);
	sys_drv_analog_reg15_set(0x0);
	sys_drv_analog_reg16_set(0x0);
	sys_drv_analog_reg17_set(0x0);

	//disable adc and dac interrupt
	aud_hal_adc_int_disable();
	aud_hal_dac_int_disable();
	//disable audio interrupt
	sys_drv_aud_int_en(0);
	delay(10);
	//ungister isr
	aud_int_config_t int_config_table = {INT_SRC_AUDIO, aud_isr};
	bk_int_isr_unregister(int_config_table.int_src);

	//reset audo configure
	bk_aud_adc_deinit();
	bk_aud_dac_deinit();
	s_aud_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_aud_set_adc_samp_rate(aud_adc_samp_rate_t samp_rate)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_set_audio_config_samp_rate_adc(samp_rate);
	return BK_OK;
}

/* get adc fifo port address */
bk_err_t bk_aud_get_adc_fifo_addr(uint32_t *adc_fifo_addr)
{
	AUD_RETURN_ON_NOT_INIT();
	*adc_fifo_addr = AUD_ADC_FPORT_ADDR;
	return BK_OK;
}

/* get dtmf fifo port address */
bk_err_t bk_aud_get_dtmf_fifo_addr(uint32_t *dtmf_fifo_addr)
{
	AUD_RETURN_ON_NOT_INIT();
	*dtmf_fifo_addr = AUD_DTMF_FPORT_ADDR;
	return BK_OK;
}

/* get dac fifo port address */
bk_err_t bk_aud_get_dac_fifo_addr(uint32_t *dac_fifo_addr)
{
	AUD_RETURN_ON_NOT_INIT();
	*dac_fifo_addr = AUD_DACL_FPORT_ADDR;
	return BK_OK;
}

/* get adc fifo data */
bk_err_t bk_aud_get_adc_fifo_data(uint32_t *adc_data)
{
	AUD_RETURN_ON_NOT_INIT();
	*adc_data = aud_hal_get_adc_fport_value();
	return BK_OK;
}

/* get dtmf fifo data */
bk_err_t bk_aud_get_dtmf_fifo_data(uint32_t *dtmf_data)
{
	AUD_RETURN_ON_NOT_INIT();
	*dtmf_data = aud_hal_get_dtmf_fport_value();
	return BK_OK;
}

/* get audio adc fifo and agc status */
bk_err_t bk_aud_get_adc_status(aud_adc_status_t *adc_status)
{
	AUD_RETURN_ON_NOT_INIT();
	if (adc_mode != AUD_ADC_WORK_MODE_ADC)
		return BK_ERR_AUD_ADC_MODE;

	if (aud_hal_get_fifo_status_adcl_near_full())
		adc_status->adcl_near_full = true;
	else
		adc_status->adcl_near_full = false;

	if (aud_hal_get_fifo_status_dtmf_near_full())
		adc_status->dtmf_near_full = true;
	else
		adc_status->dtmf_near_full = false;

	if (aud_hal_get_fifo_status_adcl_near_empty())
		adc_status->adcl_near_empty = true;
	else
		adc_status->adcl_near_empty = false;

	if (aud_hal_get_fifo_status_dtmf_near_empty())
		adc_status->dtmf_near_empty = true;
	else
		adc_status->dtmf_near_empty = false;

	if (aud_hal_get_fifo_status_adcl_fifo_full())
		adc_status->adcl_fifo_full = true;
	else
		adc_status->adcl_fifo_full = false;

	if (aud_hal_get_fifo_status_dtmf_fifo_full())
		adc_status->dtmf_fifo_full = true;
	else
		adc_status->dtmf_fifo_full = false;

	if (aud_hal_get_fifo_status_adcl_fifo_empty())
		adc_status->adcl_fifo_empty = true;
	else
		adc_status->adcl_fifo_empty = false;

	if (aud_hal_get_fifo_status_dtmf_fifo_empty())
		adc_status->dtmf_fifo_empty = true;
	else
		adc_status->dtmf_fifo_empty = false;

	adc_status->rssi_in_db = aud_hal_get_agc_status_rssi();
	adc_status->mic_pga = aud_hal_get_agc_status_mic_pga();
	adc_status->mic_rssi = aud_hal_get_agc_status_mic_rssi();

	return BK_OK;
}

/* get audio dtmf fifo status */
bk_err_t bk_aud_get_dtmf_status(aud_dtmf_status_t *dtmf_status)
{
	AUD_RETURN_ON_NOT_INIT();
	if (adc_mode != AUD_ADC_WORK_MODE_ADC)
		return BK_ERR_AUD_ADC_MODE;

	if (aud_hal_get_fifo_status_dtmf_near_full())
		dtmf_status->dtmf_near_full = true;
	else
		dtmf_status->dtmf_near_full = false;

	if (aud_hal_get_fifo_status_dtmf_near_empty())
		dtmf_status->dtmf_near_empty = true;
	else
		dtmf_status->dtmf_near_empty = false;

	if (aud_hal_get_fifo_status_dtmf_fifo_full())
		dtmf_status->dtmf_fifo_full = true;
	else
		dtmf_status->dtmf_fifo_full = false;

	if (aud_hal_get_fifo_status_dtmf_fifo_empty())
		dtmf_status->dtmf_fifo_empty = true;
	else
		dtmf_status->dtmf_fifo_empty = false;

	return BK_OK;
}

/* start adc to dac test */
bk_err_t bk_aud_start_loop_test(void)
{
	bk_err_t ret = BK_OK;
	AUD_RETURN_ON_NOT_INIT();
	switch (adc_mode){
		case AUD_ADC_WORK_MODE_NULL:
			ret = BK_ERR_AUD_ADC_MODE;
			break;
		case AUD_ADC_WORK_MODE_ADC:
			//check if DAC enable
			aud_hal_set_fifo_config_loop_adc2dac(1);
			break;
		case AUD_ADC_WORK_MODE_DTMF:
			//check if DAC enable
			aud_hal_set_fifo_config_loop_ton2dac(1);
			break;
		default:
			break;
	}

	return ret;
}

/* stop adc to dac test */
bk_err_t bk_aud_stop_loop_test(void)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_set_fifo_config_loop_ton2dac(0);
	aud_hal_set_fifo_config_loop_adc2dac(0);
	return BK_OK;
}

/* enable adc interrupt */
bk_err_t bk_aud_enable_adc_int(void)
{
	bk_err_t ret = BK_OK;
	AUD_RETURN_ON_NOT_INIT();
	sys_drv_aud_int_en(0);
	switch (adc_mode){
		case AUD_ADC_WORK_MODE_NULL:
			ret = BK_ERR_AUD_ADC_MODE;
			break;
		case AUD_ADC_WORK_MODE_ADC:
			aud_hal_adc_int_enable();
			break;
		case AUD_ADC_WORK_MODE_DTMF:
			aud_hal_set_fifo_config_dtmf_int_en(1);
			break;
		default:
			break;
	}
	sys_drv_aud_int_en(1);

	return ret;
}

/* disable adc interrupt */
bk_err_t bk_aud_disable_adc_int(void)
{
	bk_err_t ret = BK_OK;
	AUD_RETURN_ON_NOT_INIT();
	switch (adc_mode){
		case AUD_ADC_WORK_MODE_NULL:
			ret = BK_ERR_AUD_ADC_MODE;
			break;
		case AUD_ADC_WORK_MODE_ADC:
			aud_hal_adc_int_disable();
			break;
		case AUD_ADC_WORK_MODE_DTMF:
			aud_hal_set_fifo_config_dtmf_int_en(0);
			break;
		default:
			break;
	}
	return ret;
}

/* enable adc and adc start work */
bk_err_t bk_aud_start_adc(void)
{
	bk_err_t ret = BK_OK;
	AUD_RETURN_ON_NOT_INIT();
	switch (adc_mode){
		case AUD_ADC_WORK_MODE_NULL:
			ret = BK_ERR_AUD_ADC_MODE;
			break;
		case AUD_ADC_WORK_MODE_ADC:
			aud_hal_adc_enable();
			break;
		case AUD_ADC_WORK_MODE_DTMF:
			aud_hal_dtmf_enable();
			break;
		default:
			break;
	}

	return ret;
}

/* disable adc and adc stop work */
bk_err_t bk_aud_stop_adc(void)
{
	bk_err_t ret = BK_OK;
	AUD_RETURN_ON_NOT_INIT();
	switch (adc_mode){
		case AUD_ADC_WORK_MODE_NULL:
			ret = BK_ERR_AUD_ADC_MODE;
			break;
		case AUD_ADC_WORK_MODE_ADC:
			aud_hal_adc_disable();
			break;
		case AUD_ADC_WORK_MODE_DTMF:
			aud_hal_dtmf_disable();
			break;
		default:
			break;
	}

	return ret;
}

bk_err_t bk_aud_dac_init(const aud_dac_config_t *dac_config)
{
	AUD_RETURN_ON_NOT_INIT();
	BK_RETURN_ON_NULL(dac_config);

	//config system registers
	sys_drv_aud_dacdrv_en(1);
	sys_drv_aud_bias_en(1);

	//enable dacl and dacr
	sys_drv_aud_dacr_en(1);
	delay(100);
	sys_drv_aud_dacl_en(1);
	delay(100);

	aud_hal_dac_config(dac_config);

	return BK_OK;
}

bk_err_t bk_aud_dac_deinit(void)
{
	aud_hal_dac_int_disable();
	aud_hal_set_audio_config_dac_enable(0);

	return BK_OK;
}

bk_err_t bk_aud_set_dac_samp_rate(aud_dac_samp_rate_t samp_rate)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_dac_set_sample_rate(samp_rate);

	return BK_OK;
}

bk_err_t bk_aud_dac_write(uint32_t pcm_value)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_dac_write(pcm_value);

	return BK_OK;
}

bk_err_t bk_aud_set_dac_read_threshold(uint16_t dacl_throld, uint16_t dacr_throld)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_set_fifo_config_dacr_rd_threshold(dacr_throld);
	aud_hal_set_fifo_config_dacl_rd_threshold(dacl_throld);

	return BK_OK;
}

bk_err_t bk_aud_enable_dac_int(void)
{
	AUD_RETURN_ON_NOT_INIT();
	sys_drv_aud_int_en(1);
	aud_hal_dac_int_enable();

	return BK_OK;
}

bk_err_t bk_aud_disable_dac_int(void)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_dac_int_disable();

	return BK_OK;
}

bk_err_t bk_aud_get_dac_status(aud_dac_status_t *dac_status)
{
	AUD_RETURN_ON_NOT_INIT();
	dac_status->dacr_near_full = aud_hal_get_fifo_status_dacr_near_full();
	dac_status->dacl_near_full = aud_hal_get_fifo_status_dacl_near_full();
	dac_status->dacr_near_empty = aud_hal_get_fifo_status_dacr_near_empty();
	dac_status->dacl_near_empty = aud_hal_get_fifo_status_dacl_near_empty();
	dac_status->dacr_fifo_full = aud_hal_get_fifo_status_dacr_fifo_full();
	dac_status->dacl_fifo_full = aud_hal_get_fifo_status_dacl_fifo_full();
	dac_status->dacr_fifo_empty = aud_hal_get_fifo_status_dacr_fifo_empty();
	dac_status->dacl_fifo_empty = aud_hal_get_fifo_status_dacl_fifo_empty();

	return BK_OK;
}

bk_err_t bk_aud_start_dac(void)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_set_audio_config_dac_enable(1);

	return BK_OK;
}

bk_err_t bk_aud_stop_dac(void)
{
	AUD_RETURN_ON_NOT_INIT();
	aud_hal_set_audio_config_dac_enable(0);

	return BK_OK;
}

/* register audio interrupt */
bk_err_t bk_aud_register_aud_isr(aud_isr_id_t isr_id, aud_isr_t isr, void *param)
{
	AUD_RETURN_ON_INVALID_ISR_ID(isr_id);
	uint32_t int_level = rtos_disable_int();
	s_aud_isr[isr_id].callback = isr;
	s_aud_isr[isr_id].param = param;
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_aud_cpu_int_en(uint32_t value)
{
	sys_drv_aud_int_en(value);

	return BK_OK;
}

/* audio check interrupt flag and excute correponding isr function when enter interrupt */
static void aud_isr_common(void)
{
	uint32_t adcl_int_status = aud_hal_get_fifo_status_adcl_int_flag();
	uint32_t dtmf_int_status = aud_hal_get_fifo_status_dtmf_int_flag();

	uint32_t dacl_int_status = aud_hal_get_fifo_status_dacl_int_flag();
	uint32_t dacr_int_status = aud_hal_get_fifo_status_dacr_int_flag();
	//os_printf("enter isr\r\n");
	//AUD_LOGE("adcl_int_status:%d, dtmf_int_status:%d, dacr_int_status:%d, dacl_int_status:%d\r\n", adcl_int_status, dtmf_int_status, dacr_int_status, dacl_int_status);

	if (adcl_int_status) {
		if (s_aud_isr[AUD_ISR_ADCL].callback) {
			s_aud_isr[AUD_ISR_ADCL].callback(s_aud_isr[AUD_ISR_ADCL].param);
		}
	}

	if (dtmf_int_status) {
		if (s_aud_isr[AUD_ISR_DTMF].callback) {
			s_aud_isr[AUD_ISR_DTMF].callback(s_aud_isr[AUD_ISR_DTMF].param);
		}
	}

	if (dacl_int_status) {
		if (s_aud_isr[AUD_ISR_DACL].callback) {
			s_aud_isr[AUD_ISR_DACL].callback(s_aud_isr[AUD_ISR_DACL].param);
		}
	}

	if (dacr_int_status) {
		if (s_aud_isr[AUD_ISR_DACR].callback) {
			s_aud_isr[AUD_ISR_DACR].callback(s_aud_isr[AUD_ISR_DACR].param);
		}
	}
}

/* audio interrupt enter*/
static void aud_isr(void)
{
	aud_isr_common();
}

