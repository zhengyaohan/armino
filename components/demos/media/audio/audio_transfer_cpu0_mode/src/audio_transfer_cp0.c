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

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/aud.h>
#include <driver/aud_types.h>
#include <driver/dma.h>
#include "aud_hal.h"
#include "sys_driver.h"
#include "aud_driver.h"
#include "audio_transfer_types.h"
#include <modules/aec.h>
#include <modules/audio_ring_buff.h>
#include <modules/g711.h>

#define  AUD_TRANS_DEBUG_DAC  0
#define  AUD_TRANS_DEBUG_ADC  0
#define  AUD_DEBUG  0


typedef enum {
	ADC_TEST_MODE_NULL = 0,
	ADC_TEST_MODE_MCP,
	ADC_TEST_MODE_DMA,
	ADC_TEST_MODE_MAX
} aud_test_mode_t;


#define TU_QITEM_COUNT      (60)
static beken_thread_t  audio_thread_hdl = NULL;
static beken_queue_t aud_int_msg_que = NULL;


AECContext* aec = NULL;

uint32_t val = 0;
int16_t samplerate = 8000;
uint32_t aec_context_size = 0;
static uint32_t frame_sample = 0;        //一帧AEC处理数据的点数
int16_t* ref_addr = NULL;
int16_t* mic_addr = NULL;
int16_t* out_addr = NULL;
int32_t *mic_ring_buff = NULL;     //存放audio adc采集到的mic信号
int32_t *ref_ring_buff = NULL;     //存放aec的ref信号
int16_t *aec_ring_buff = NULL;         //存放经过aec算法处理后的mic信号
int32_t *speaker_ring_buff = NULL;     //存放经过decoder解码后的dac信号
int8_t *decoder_ring_buff = NULL;     //存放需要decoder解码的dac信号

static dma_id_t adc_dma_id = DMA_ID_MAX;
static dma_id_t dac_dma_id = DMA_ID_MAX;
static RingBufferContext mic_rb;
static RingBufferContext ref_rb;
static RingBufferContext aec_rb;
static RingBufferContext decoder_rb;
static RingBufferContext speaker_rb;


int (*audio_send_audio_data)(unsigned char *data, unsigned int len) = NULL;
void (*audio_transfer_init_callback)(void) = NULL;

extern void delay(int num);

static bk_err_t audio_send_msg(audio_transfer_msg_t msg)
{
	bk_err_t ret;

	if (aud_int_msg_que) {
		ret = rtos_push_to_queue(&aud_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			os_printf("audio_send_msg failed\r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t audio_send_stop_msg(void)
{
	bk_err_t ret;
	audio_transfer_msg_t msg;
	msg.op = AUDIO_TRANSFER_EXIT;

	if (aud_int_msg_que) {
		ret = rtos_push_to_queue_front(&aud_int_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			os_printf("audio_send_stop_msg failed\r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}


static bk_err_t audio_adc_config(audio_sample_rate_t samp_rate)
{
	bk_err_t ret = BK_OK;
	aud_adc_config_t adc_config;

	adc_config.mic_config = AUD_MIC_MIC1_ENABLE;
	adc_config.samp_rate = samp_rate;
	adc_config.adc_enable = AUD_ADC_DISABLE;
	adc_config.line_enable = AUD_ADC_LINE_DISABLE;
	adc_config.dtmf_enable = AUD_DTMF_DISABLE;
	adc_config.adc_hpf2_coef_B2 = 0;
	adc_config.adc_hpf2_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
	adc_config.adc_hpf1_bypass_enable = AUD_ADC_HPF_BYPASS_ENABLE;
	adc_config.adc_set_gain = 0x2d;
	adc_config.adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING;
	adc_config.adc_hpf2_coef_B0 = 0;
	adc_config.adc_hpf2_coef_B1 = 0;
	adc_config.adc_hpf2_coef_A0 = 0;
	adc_config.adc_hpf2_coef_A1 = 0;
	adc_config.dtmf_wr_threshold = 8;
	adc_config.adcl_wr_threshold = 8;
	adc_config.dtmf_int_enable = AUD_DTMF_INT_DISABLE;
	adc_config.adcl_int_enable = AUD_ADCL_INT_DISABLE;
	adc_config.loop_adc2dac = AUD_LOOP_ADC2DAC_DISABLE;
	adc_config.agc_noise_thrd = 101;
	adc_config.agc_noise_high = 101;
	adc_config.agc_noise_low = 160;
	adc_config.agc_noise_min = 1;
	adc_config.agc_noise_tout = 0;
	adc_config.agc_high_dur = 3;
	adc_config.agc_low_dur = 3;
	adc_config.agc_min = 1;
	adc_config.agc_max = 4;
	adc_config.agc_ng_method = AUD_AGC_NG_METHOD_MUTE;
	adc_config.agc_ng_enable = AUD_AGC_NG_DISABLE;
	adc_config.agc_decay_time = AUD_AGC_DECAY_TIME_128;
	adc_config.agc_attack_time = AUD_AGC_ATTACK_TIME_128;
	adc_config.agc_high_thrd = 18;
	adc_config.agc_low_thrd = 0;
	adc_config.agc_iir_coef = AUD_AGC_IIR_COEF_1_1024;
	adc_config.agc_enable = AUD_AGC_DISABLE;
	adc_config.manual_pga_value = 0;
	adc_config.manual_pga_enable = AUD_GAC_MANUAL_PGA_DISABLE;
	adc_config.adc_fracmod_manual = AUD_ADC_TRACMOD_MANUAL_DISABLE;
	adc_config.adc_fracmod = 0;

	/* init audio driver and config adc */
	ret = bk_aud_driver_init();
	if (ret != BK_OK) {
		os_printf("init audio driver fail \r\n");
		goto aud_adc_exit;
	}

	ret = bk_aud_adc_init(AUD_ADC_WORK_MODE_ADC, &adc_config, NULL);
	if (ret != BK_OK) {
		os_printf("init audio adc fail \r\n");
		goto aud_adc_exit;
	}

	return BK_OK;

aud_adc_exit:
	os_printf("audio adc config fail \r\n");
	bk_aud_driver_deinit();
	return BK_FAIL;
}

static bk_err_t audio_dac_config(audio_sample_rate_t samp_rate)
{
	bk_err_t ret = BK_OK;
	aud_dac_config_t dac_config;

	dac_config.dac_enable = AUD_DAC_DISABLE;
	//dac_config.samp_rate = samp_rate;
	//dac_config.samp_rate = AUDIO_SAMP_RATE_8K;
	dac_config.samp_rate = AUD_DAC_SAMP_RATE_SOURCE_8K;
	dac_config.dac_hpf2_coef_B2 = 0x3A22;
	dac_config.dac_hpf2_bypass_enable = AUD_DAC_HPF_BYPASS_ENABLE;
	dac_config.dac_hpf1_bypass_enable = AUD_DAC_HPF_BYPASS_ENABLE;
	dac_config.dac_set_gain = 0x2D;    //default 2D  3F  15
	dac_config.dac_clk_invert = AUD_DAC_CLK_INVERT_RISING;

	dac_config.dac_hpf2_coef_B0 = 0x3A22;
	dac_config.dac_hpf2_coef_B1 = 0x8BBF;
	dac_config.dac_hpf2_coef_A1 = 0x751C;
	dac_config.dac_hpf2_coef_A2 = 0xC9E6;
	dac_config.dacr_rd_threshold = 0x4;
	dac_config.dacl_rd_threshold = 0x4;
	dac_config.dacr_int_enable = 0x0;
	dac_config.dacl_int_enable = 0x0;
	dac_config.dac_filt_enable = AUD_DAC_FILT_DISABLE;
	dac_config.dac_fracmod_manual_enable = AUD_DAC_FRACMOD_MANUAL_DISABLE;
	dac_config.dac_fracmode_value = 0x0;

	/* init audio driver and config dac */
	ret = bk_aud_driver_init();
	if (ret != BK_OK) {
		os_printf("init audio driver fail \r\n");
		goto aud_dac_exit;
	}

	ret = bk_aud_dac_init(&dac_config);
	if (ret != BK_OK) {
		os_printf("init audio dac fail \r\n");
		goto aud_dac_exit;
	}

	return BK_OK;

aud_dac_exit:
	os_printf("audio dac config fail \r\n");
	bk_aud_driver_deinit();
	return BK_FAIL;
}

static void audio_aec_config(audio_sample_rate_t samp_rate)
{
	/* init aec context */
	aec_context_size = aec_size();
	aec = (AECContext*)os_malloc(aec_context_size);

	/* config sample rate, default is 8K */
	if (samp_rate == AUDIO_SAMP_RATE_16K)
		aec_init(aec, 16000);
	else
		aec_init(aec, 8000);

	/* 获取处理帧长，16000采样率320点(640字节)，8000采样率160点(320字节)  (对应20毫秒数据) */
	aec_ctrl(aec, AEC_CTRL_CMD_GET_FRAME_SAMPLE, (uint32_t)(&frame_sample));

	/* 获取结构体内部可以复用的ram作为每帧tx,rx,out数据的临时buffer; ram很宽裕的话也可以在外部单独申请获取 */
	aec_ctrl(aec, AEC_CTRL_CMD_GET_TX_BUF, (uint32_t)(&val)); mic_addr = (int16_t*)val;
	aec_ctrl(aec, AEC_CTRL_CMD_GET_RX_BUF, (uint32_t)(&val)); ref_addr = (int16_t*)val;
	aec_ctrl(aec, AEC_CTRL_CMD_GET_OUT_BUF,(uint32_t)(&val)); out_addr = (int16_t*)val;

	/* 以下是参数调节示例,aec_init中都已经有默认值,可以直接先用默认值 */
	aec_ctrl(aec, AEC_CTRL_CMD_SET_FLAGS, 0x1f);							  //库内各模块开关; aec_init内默认赋值0x1f;

	/* 回声消除相关 */
	aec_ctrl(aec, AEC_CTRL_CMD_SET_MIC_DELAY, 0);							  //设置参考信号延迟(采样点数，需要dump数据观察)
	aec_ctrl(aec, AEC_CTRL_CMD_SET_EC_DEPTH, 0);							  //建议取值范围1~50; 后面几个参数建议先用aec_init内的默认值，具体需要根据实际情况调试; 总得来说回声越大需要调的越大
	aec_ctrl(aec, AEC_CTRL_CMD_SET_TxRxThr, 60);							  //建议取值范围10~64
	aec_ctrl(aec, AEC_CTRL_CMD_SET_TxRxFlr, 40);							  //建议取值范围1~10
	aec_ctrl(aec, AEC_CTRL_CMD_SET_REF_SCALE, 0);							  //取值0,1,2；rx数据如果幅值太小的话适当放大
	aec_ctrl(aec, AEC_CTRL_CMD_SET_VOL, 14);								  //通话过程中如果需要经常调节喇叭音量就设置下当前音量等级
	/* 降噪相关 */
	aec_ctrl(aec, AEC_CTRL_CMD_SET_NS_LEVEL, 2);							  //建议取值范围1~8；值越小底噪越小
	aec_ctrl(aec, AEC_CTRL_CMD_SET_NS_PARA,  1);							  //只能取值0,1,2; 降噪由弱到强，建议默认值
	/* drc(输出音量相关) */
	aec_ctrl(aec, AEC_CTRL_CMD_SET_DRC, 0x15);								  //建议取值范围0x10~0x1f;   越大输出声音越大

}

/* 搬运audio adc 采集到的一帧mic和ref信号后，触发中断通知AEC处理数据 */
static void audio_dma_adc_finish_isr(void)
{
	//os_printf("cp1: enter the audio_dma_adc_finish_isr \r\n");
	bk_err_t ret = BK_OK;
	audio_transfer_msg_t msg;

	/* send msg to AEC to process mic and ref data */
	msg.op = AUDIO_TRANSFER_AEC;
	ret = audio_send_msg(msg);
	if (ret != kNoErr) {
		os_printf("send msg: %d fails \r\n", msg.op);
	}
}

static bk_err_t audio_adc_dma_config(dma_id_t dma_id, int32_t *ring_buff_addr, uint32_t ring_buff_size, uint32_t transfer_len)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t adc_port_addr;

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_AUDIO;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_16BITS;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;

	/* get adc fifo address */
	if (bk_aud_get_adc_fifo_addr(&adc_port_addr) != BK_OK) {
		os_printf("get adc fifo address failed\r\n");
		return BK_FAIL;
	} else {
		dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
		dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		dma_config.src.start_addr = adc_port_addr;
		dma_config.src.end_addr = adc_port_addr + 4;
	}

	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)ring_buff_addr;
	dma_config.dst.end_addr = (uint32_t)ring_buff_addr + ring_buff_size;

	/* init dma channel */
	ret = bk_dma_init(dma_id, &dma_config);
	if (ret != BK_OK) {
		os_printf("audio adc dma channel init fail \r\n");
		return BK_FAIL;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(dma_id, transfer_len);

	//register isr
	bk_dma_register_isr(dma_id, NULL, (void *)audio_dma_adc_finish_isr);
	bk_dma_enable_finish_interrupt(dma_id);

	return BK_OK;
}

/* 搬运audio dac 一帧dac信号后，触发中断通知decoder处理数据 */
static void audio_dma_dac_finish_isr(void)
{
#if 0
	//os_printf("cp1: enter the audio_dma_dac_finish_isr \r\n");
	bk_err_t ret = BK_OK;
	audio_transfer_msg_t msg;

	/* send msg to decoder to decoding recevied data */
	msg.op = AUDIO_TRANSFER_DECODER;
	ret = audio_send_msg(msg);
	if (ret != kNoErr) {
		os_printf("send msg: %d fails \r\n", msg.op);
	}
#endif
}

static bk_err_t audio_dac_dma_config(dma_id_t dma_id, int32_t *ring_buff_addr, uint32_t ring_buff_size, uint32_t transfer_len)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config;
	uint32_t dac_port_addr;

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 1;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.dst.dev = DMA_DEV_AUDIO;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
	/* get dac fifo address */
	if (bk_aud_get_dac_fifo_addr(&dac_port_addr) != BK_OK) {
		os_printf("get dac fifo address failed\r\n");
		return BK_FAIL;
	} else {
		dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
		dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
		dma_config.dst.start_addr = dac_port_addr;
		dma_config.dst.end_addr = dac_port_addr + 4;
	}
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	//dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.src.start_addr = (uint32_t)ring_buff_addr;
	dma_config.src.end_addr = (uint32_t)(ring_buff_addr) + ring_buff_size;

	/* init dma channel */
	ret = bk_dma_init(dma_id, &dma_config);
	if (ret != BK_OK) {
		os_printf("audio dac dma channel init fail \r\n");
		return BK_FAIL;
	}

	/* set dma transfer length */
	bk_dma_set_transfer_len(dma_id, transfer_len);

	//register isr
	bk_dma_register_isr(dma_id, NULL, (void *)audio_dma_dac_finish_isr);
	bk_dma_enable_finish_interrupt(dma_id);

	return BK_OK;
}

static bk_err_t audio_aec_process(void)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;
	audio_transfer_msg_t msg;

	/* get a fram mic data from mic_ring_buff */
	size = ring_buffer_read(&mic_rb, (uint8_t*)mic_addr, frame_sample*2);
	if (size != frame_sample*2) {
		os_printf("the mic data readed from mic_ring_buff is not a frame \r\n");
		//return BK_FAIL;
	}

	/* read ref data from ref_ring_buff */
	if (ring_buffer_get_fill_size(&ref_rb) >= frame_sample*2) {
		size = ring_buffer_read(&ref_rb, (uint8_t*)ref_addr, frame_sample*2);
		if (size != frame_sample*2) {
			os_printf("the ref data readed from ref_ring_buff is not a frame \r\n");
			//return BK_FAIL;
			//os_memcpy(ref_addr, 0, frame_sample*2);
		}
	}

	/* aec process data */
	aec_proc(aec, ref_addr, mic_addr, out_addr);

	/* save mic data after aec processed to aec_ring_buffer */
	if (ring_buffer_get_free_size(&aec_rb) >= frame_sample*2) {
		size = ring_buffer_write(&aec_rb, (uint8_t*)out_addr, frame_sample*2);
		if (size != frame_sample*2) {
			os_printf("the data writeten to aec_ring_buff is not a frame \r\n");
			//return BK_FAIL;
		}
	}

	/* send msg to encoder to encoding data */
	msg.op = AUDIO_TRANSFER_ENCODER;
	ret = audio_send_msg(msg);
	if (ret != kNoErr) {
		os_printf("send msg: %d fails \r\n", msg.op);
		return BK_FAIL;
	}

	return ret;
}

static bk_err_t audio_encoder_process(void)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;
	uint32_t i = 0;
	int16_t pcm_data[AUD_AEC_8K_FRAME_SAMP_SIZE/2] = {0};
	unsigned char law_data[AUD_AEC_8K_FRAME_SAMP_SIZE/2] = {0};

	/* get data from aec_ring_buff */
	size = ring_buffer_read(&aec_rb, (uint8_t *)pcm_data, frame_sample*2);
	if (size != frame_sample*2) {
		os_printf("the data readed from aec_ring_buff is not a frame \r\n");
		return BK_FAIL;
	}

	/* G711A encoding pcm data to a-law data*/
	for (i=0; i<frame_sample; i++) {
		law_data[i] = linear2alaw(pcm_data[i]);
	}

	/* send audio data to apk */
	if (!audio_send_audio_data) {
		os_printf("not register send data api \r\n");
	} else {
		size = audio_send_audio_data(law_data, frame_sample);
		if (size != frame_sample) {
			os_printf("send audio data to apk fial by wifi \r\n");
			ret = BK_FAIL;
		}
	}

	return ret;
}

bk_err_t bk_audio_write_data(uint8_t *data_buf, uint32_t length)
{
	uint32_t size = 0;
	bk_err_t ret = BK_OK;
	audio_transfer_msg_t msg;

	size = ring_buffer_get_free_size(&decoder_rb);
	if (size >= length) {
		size = ring_buffer_write(&decoder_rb, data_buf, length);
		if (size != length) {
			os_printf("%s write data to decoder_rb error \r\n", __func__);
			return BK_FAIL;
		}
	} else {
		os_printf("decoder_rb is not enough, free:%d, write:%d\r\n", size, length);
		return BK_FAIL;
	}

	/* send msg to task to decoder data */
	msg.op = AUDIO_TRANSFER_DECODER;
	ret = audio_send_msg(msg);
	if (ret != kNoErr) {
		os_printf("send msg: %d fails \r\n", msg.op);
		return BK_FAIL;
	}

	return ret;
}

static bk_err_t audio_decoder_process(void)
{
	uint32_t size = 0;
	uint32_t fill_size = 0;
	uint32_t i = 0;
	unsigned char law_data[AUD_DECD_FRAME_SAMP_SIZE] = {0};
	//uint32_t mic_fill_size = 0;
	//uint32_t speaker_fill_size = 0;
	//uint32_t ref_fill_size = 0;
	int16_t pcm_data[AUD_DECD_FRAME_SAMP_SIZE] = {0};

	fill_size = ring_buffer_get_fill_size(&decoder_rb);
	if (fill_size >= AUD_DECD_FRAME_SAMP_SIZE) {

		/* get data from decoder_ring_buff */
		size = ring_buffer_read(&decoder_rb, (uint8_t*)law_data, AUD_DECD_FRAME_SAMP_SIZE);
		if (size != AUD_DECD_FRAME_SAMP_SIZE) {
			os_printf("the data readed from decoder_ring_buff is not a frame \r\n");
			return BK_FAIL;
		}

		/* G711A decoding a-law data to pcm data*/
		for (i=0; i<AUD_DECD_FRAME_SAMP_SIZE; i++) {
			pcm_data[i] = alaw2linear(law_data[i]);
		}
#if 0
		/* read mic fill data size */
		mic_fill_size = ring_buffer_get_fill_size(&mic_rb);
		//os_printf("mic_rb: fill_size=%d \r\n", mic_fill_size);
		speaker_fill_size = ring_buffer_get_fill_size(&speaker_rb);
		//os_printf("speaker_rb: fill_size=%d \r\n", speaker_fill_size);
		ref_fill_size = ring_buffer_get_fill_size(&ref_rb);
		//os_printf("ref_rb: fill_size=%d \r\n", ref_fill_size);
		/* 设置参考信号延迟(采样点数，需要dump数据观察) */
		//os_printf("MIC_DELAY: %d \r\n", (mic_fill_size + speaker_fill_size - ref_fill_size)/2);
		if ((mic_fill_size + speaker_fill_size - ref_fill_size)/2 < 0) {
			os_printf("MIC_DELAY is error \r\n", ref_fill_size);
			aec_ctrl(aec, AEC_CTRL_CMD_SET_MIC_DELAY, 272);
		} else {
			aec_ctrl(aec, AEC_CTRL_CMD_SET_MIC_DELAY, (mic_fill_size + speaker_fill_size - ref_fill_size)/2);
		}
#endif
		/* save the data after G711A processed to encoder_ring_buffer */
		if (ring_buffer_get_free_size(&speaker_rb) > AUD_DECD_FRAME_SAMP_SIZE*2) {
			size = ring_buffer_write(&speaker_rb, (uint8_t *)pcm_data, AUD_DECD_FRAME_SAMP_SIZE*2);
			if (size != AUD_DECD_FRAME_SAMP_SIZE*2) {
				os_printf("the data writeten to speaker_ring_buff is not a frame, size=%d \r\n", size);
				return BK_FAIL;
			}
		}

		if (ring_buffer_get_free_size(&ref_rb) > AUD_DECD_FRAME_SAMP_SIZE*2) {
			size = ring_buffer_write(&ref_rb, (uint8_t *)pcm_data, AUD_DECD_FRAME_SAMP_SIZE*2);
			if (size != AUD_DECD_FRAME_SAMP_SIZE*2) {
				os_printf("the data writeten to ref_ring_buff is not a frame, size=%d \r\n", size);
				return BK_FAIL;
			}
		}
	}else {
		//TODO
	}

	return BK_OK;
}

/* register callbacks of audio */
void bk_audio_register_rw_cb(int (*send_audio_data)(unsigned char *data, unsigned int len), void (*transfer_init_callback)(void))
{
	audio_send_audio_data = send_audio_data;
	audio_transfer_init_callback = transfer_init_callback;
}

static void audio_start_transfer_process(void)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;
	uint8_t pcm_data[AUD_DECD_FRAME_SAMP_SIZE*2]= {0};

	/* start DMA */
	ret = bk_dma_start(adc_dma_id);
	if (ret != BK_OK) {
		os_printf("start adc dma fail \r\n");
		return;
	}

	/* enable adc */
	/* wait receive data and then open adc */
	bk_aud_start_adc();

	/* enable dac */
	bk_aud_start_dac();

	ret = bk_dma_start(dac_dma_id);
	if (ret != BK_OK) {
		os_printf("start dac dma fail \r\n");
		return;
	}

	/* write two frame data to speaker and ref ring buffer */
	size = ring_buffer_write(&speaker_rb, (uint8_t *)pcm_data, AUD_DECD_FRAME_SAMP_SIZE*2);
	if (size != AUD_DECD_FRAME_SAMP_SIZE*2) {
		os_printf("the data writeten to speaker_ring_buff error, size=%d \r\n", size);
		return;
	}

	size = ring_buffer_write(&speaker_rb, (uint8_t *)pcm_data, AUD_DECD_FRAME_SAMP_SIZE*2);
	if (size != AUD_DECD_FRAME_SAMP_SIZE*2) {
		os_printf("the data writeten to speaker_ring_buff error, size=%d \r\n", size);
		return;
	}
	os_printf("enable audio and dma to start audio transfer complete \r\n");
}

static void audio_transfer_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	audio_setup_t *audio_setup = NULL;

	/*  -------------------------step1: init audio and config ADC and DAC -------------------------------- */
	audio_setup = (audio_setup_t *)(int)param_data;
	ret = audio_adc_config(audio_setup->samp_rate);
	if (ret != BK_OK) {
		os_printf("audio adc init fail \r\n");
		return;
	}

	ret = audio_dac_config(audio_setup->samp_rate);
	if (ret != BK_OK) {
		os_printf("audio dac init fail \r\n");
		return;
	}
	os_printf("step1: init audio and config ADC and DAC complete \r\n");

	/*  -------------------------step2: init AEC and malloc two ring buffers -------------------------------- */
	/* init aec and config aec */
	audio_aec_config(audio_setup->samp_rate);
	os_printf("frame_sample: %d \r\n", frame_sample);
	frame_sample = 160;

	/* malloc mic and ref ring buffer (AEC_Frame_Size=frame_sample*2) */
	os_printf("mic_ring_buff: %d \r\n", frame_sample*2*2);
	mic_ring_buff = os_malloc(frame_sample*2*2);
	if (mic_ring_buff == NULL) {
		os_printf("malloc mic ring buffer fail \r\n");
		goto audio_transfer_exit;
	}

	os_printf("ref_ring_buff: %d \r\n", AUD_DECD_FRAME_SAMP_SIZE*2*2);
	ref_ring_buff = os_malloc(AUD_DECD_FRAME_SAMP_SIZE*2*2);
	if (ref_ring_buff == NULL) {
		os_printf("malloc ref ring buffer fail \r\n");
		goto audio_transfer_exit;
	}

	/* malloc aec ring buffer to save mic data has been aec processed */
	aec_ring_buff = os_malloc(frame_sample*2*2);
	if (aec_ring_buff == NULL) {
		os_printf("malloc aec ring buffer fail \r\n");
		goto audio_transfer_exit;
	}
	os_printf("step2: init AEC and malloc two ring buffers complete \r\n");

	/*  -------------------step3: init and config DMA to carry mic and ref data ----------------------------- */
	/* init dma driver */
	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		os_printf("dma driver init failed\r\n");
		goto audio_transfer_exit;
	}

	/* allocate free DMA channel */
	adc_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((adc_dma_id < DMA_ID_0) || (adc_dma_id >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		goto audio_transfer_exit;
	}
	os_printf("adc_dma_id: %d \r\n", adc_dma_id);

	/* config audio adc dma to carry mic data to "mic_ring_buff" */
	ret = audio_adc_dma_config(adc_dma_id, mic_ring_buff, frame_sample*2*2, frame_sample*2);
	if (ret != BK_OK) {
		os_printf("config audio adc dma fail \r\n");
		goto audio_transfer_exit;
	}
	os_printf("step3: init and config DMA to carry mic and ref data complete \r\n");

	/*  -------------------------step4: init and config G711 decoder, malloc memory and speaker ring buffer -------------------------------- */
	/* init and config G711 decoder */

	/* malloc decoder ring buffer (size: 2*AUD_DECD_FRAME_SAMP_SIZE) */
	decoder_ring_buff = os_malloc(AUD_DECD_FRAME_SAMP_SIZE*2);
	if (decoder_ring_buff == NULL) {
		os_printf("malloc decoder ring buffer fail \r\n");
		goto audio_transfer_exit;
	}

	/* malloc speaker ring buffer (size: 2*PSRAM_AUD_DECD_RING_BUFF_SIZE) */
	speaker_ring_buff = os_malloc(AUD_DECD_FRAME_SAMP_SIZE*2*2);
	if (speaker_ring_buff == NULL) {
		os_printf("malloc speaker ring buffer fail \r\n");
		goto audio_transfer_exit;
	}
	os_printf("step4: init and config G711 decoder, decoder ring buffer:%p and speaker ring buffer:%p complete \r\n", decoder_ring_buff, speaker_ring_buff);

	/*  -------------------step5: init and config DMA to carry dac data ----------------------------- */
	/* init dma driver */
	ret = bk_dma_driver_init();
	if (ret != BK_OK) {
		os_printf("dma driver init failed\r\n");
		goto audio_transfer_exit;
	}

	/* allocate free DMA channel */
	dac_dma_id = bk_dma_alloc(DMA_DEV_AUDIO);
	if ((dac_dma_id < DMA_ID_0) || (dac_dma_id >= DMA_ID_MAX)) {
		os_printf("malloc dma fail \r\n");
		return;
	}
	os_printf("dac_dma_id: %d \r\n", dac_dma_id);

	/* config audio dac dma to carry dac data to "speaker_ring_buff" */
	ret = audio_dac_dma_config(dac_dma_id, speaker_ring_buff, AUD_DECD_FRAME_SAMP_SIZE*2*2, AUD_DECD_FRAME_SAMP_SIZE*2);
	if (ret != BK_OK) {
		os_printf("config audio dac dma fail \r\n");
		return;
	}
	os_printf("step5: init and config DMA to carry dac data complete \r\n");

	/*  -------------------------step6: init all audio ring buffers -------------------------------- */
	/* init mic_ring_buff */
	ring_buffer_init(&mic_rb, (uint8_t*)mic_ring_buff, frame_sample*2*2, adc_dma_id, RB_DMA_TYPE_WRITE);
	/* init ref_ring_buff */
	ring_buffer_init(&ref_rb, (uint8_t*)ref_ring_buff, AUD_DECD_FRAME_SAMP_SIZE*2*2, adc_dma_id, RB_DMA_TYPE_NULL);

	/* init aec_ring_buff */
	ring_buffer_init(&aec_rb, (uint8_t*)aec_ring_buff, frame_sample*2*2, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	/* init decoder_ring_buff */
	ring_buffer_init(&decoder_rb, (uint8_t*)decoder_ring_buff, AUD_DECD_FRAME_SAMP_SIZE*2, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	/* init speaker_ref_ring_buff */
	ring_buffer_init(&speaker_rb, (uint8_t*)speaker_ring_buff, AUD_DECD_FRAME_SAMP_SIZE*2*2, dac_dma_id, RB_DMA_TYPE_READ);
	os_printf("step6: init all audio ring buffers complete \r\n");

	audio_start_transfer_process();
	os_printf("init complete \r\n");

	/* init complete, and callback */
	if (!audio_transfer_init_callback) {
		os_printf("not register audio transfer init callback api \r\n");
	} else {
		audio_transfer_init_callback();
	}

	while(1) {
		audio_transfer_msg_t msg;
		ret = rtos_pop_from_queue(&aud_int_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case AUDIO_TRANSFER_IDLE:
					break;

/*
				case AUDIO_TRANSFER_START:
					audio_start_transfer_process();
					break;
*/

				case AUDIO_TRANSFER_AEC:
					audio_aec_process();
					break;

				case AUDIO_TRANSFER_ENCODER:
					ret = audio_encoder_process();
					if (ret != BK_OK) {
						os_printf("cp1: audio_encoder_process excute fail \r\n");
					}
					break;

				case AUDIO_TRANSFER_DECODER:
					audio_decoder_process();
					break;

				case AUDIO_TRANSFER_EXIT:
					goto audio_transfer_exit;
					break;

				default:
					break;
			}
		}
	}

audio_transfer_exit:
	/* disable audio adc and dac */
	bk_aud_stop_adc();
	bk_aud_adc_deinit();
	bk_aud_stop_dac();
	bk_aud_dac_deinit();
	bk_aud_driver_deinit();

	/* stop dma */
	bk_dma_stop(adc_dma_id);
	bk_dma_stop(dac_dma_id);
	bk_dma_deinit(dac_dma_id);
	bk_dma_deinit(adc_dma_id);
	bk_dma_free(DMA_DEV_AUDIO, adc_dma_id);
	bk_dma_free(DMA_DEV_AUDIO, dac_dma_id);

	/* disable AEC */
	os_free(aec);

	/* free audio ring buffer */
	ring_buffer_clear(&mic_rb);
	ring_buffer_clear(&ref_rb);
	ring_buffer_clear(&aec_rb);
	ring_buffer_clear(&decoder_rb);
	ring_buffer_clear(&speaker_rb);
	os_free(mic_ring_buff);
	os_free(ref_ring_buff);
	os_free(aec_ring_buff);
	os_free(speaker_ring_buff);

	/* delate msg queue */
	ret = rtos_deinit_queue(&aud_int_msg_que);
	if (ret != kNoErr) {
		os_printf("delate message queue fail \r\n");
		//return BK_FAIL;
	}
	aud_int_msg_que = NULL;
	os_printf("delate message queue complete \r\n");

	/* delate task */
	audio_thread_hdl = NULL;
	rtos_delete_thread(NULL);
	os_printf("delate task \r\n");
}

audio_setup_t audio_transfer_setup_bak = {0};

bk_err_t bk_audio_transfer_init(audio_setup_t *setup_cfg)
{
	bk_err_t ret = BK_OK;
	if ((!audio_thread_hdl) && (!aud_int_msg_que)) {
		os_printf("start audio_transfer test \r\n");
		os_memcpy(&audio_transfer_setup_bak, setup_cfg, sizeof(audio_setup_t));

		ret = rtos_init_queue(&aud_int_msg_que,
							  "audio_internal_queue",
							  sizeof(audio_transfer_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr) {
			os_printf("ceate audio internal message queue failed \r\n");
			return BK_FAIL;
		}

		os_printf("ceate audio internal message queue complete \r\n");

		//creat audio transfer task
		ret = rtos_create_thread(&audio_thread_hdl,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "audio_intf",
							 (beken_thread_function_t)audio_transfer_main,
							 4096,
							 (beken_thread_arg_t)&audio_transfer_setup_bak);
		if (ret != kNoErr) {
			os_printf("create audio transfer task fail \r\n");
			rtos_deinit_queue(&aud_int_msg_que);
			aud_int_msg_que = NULL;
			audio_thread_hdl = NULL;
		}
		os_printf("create audio transfer task complete \r\n");

		return kNoErr;
	} else
		return kInProgressErr;
}

bk_err_t bk_audio_transfer_deinit(void)
{
	bk_err_t ret = BK_OK;

	ret = audio_send_stop_msg();
	if (ret != BK_OK) {
		os_printf("send mailbox msg to stop audio transfer fail \r\n");
		return BK_FAIL;
	}
	os_printf(" task delate complete \r\n");

	return BK_OK;
}

