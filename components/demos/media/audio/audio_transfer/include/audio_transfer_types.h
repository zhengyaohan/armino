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
#include "audio_transfer_driver.h"
#include <driver/aud_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#define psram_block_num    10
#define PSRAM_AUD_ADDR_BASE    0x60000000

#define AUD_AEC_8K_FRAME_SAMP_SIZE           160*2
#define AUD_AEC_16K_FRAME_SAMP_SIZE          AUD_AEC_8K_FRAME_SAMP_SIZE*2
#define PSRAM_AUD_ENCD_RING_BUFF_BASE        0x60000000
#define PSRAM_AUD_8K_ENCD_RING_BUFF_SIZE     AUD_AEC_8K_FRAME_SAMP_SIZE        //byte G711压缩后size小一倍
#define PSRAM_AUD_16K_ENCD_RING_BUFF_SIZE    AUD_AEC_8K_FRAME_SAMP_SIZE*2        //byte G711压缩后size小一倍
//#define PSRAM_AUD_8K_ENCD_TEMP_BUFF          AUD_AEC_8K_FRAME_SAMP_SIZE
//#define PSRAM_AUD_16K_ENCD_TEMP_BUFF         AUD_AEC_16K_FRAME_SAMP_SIZE

/* G711 decoder 每帧数据为一个字节，所以可以设置很小来节约memory，但是从系统task调度考虑，不易太小，*/
#define PSRAM_AUD_DECD_RING_BUFF_BASE        PSRAM_AUD_ENCD_RING_BUFF_BASE + AUD_AEC_8K_FRAME_SAMP_SIZE*2
#define PSRAM_AUD_DECD_RING_BUFF_SIZE        640     //根据解码器decoder每帧数据处理的尺寸设置，2*DECD_Frame_Size
#define AUD_DECD_FRAME_SAMP_SIZE             PSRAM_AUD_DECD_RING_BUFF_SIZE/2
#endif

typedef enum {
	AUD_TRAS_SAMP_RATE_8K = 0,
	AUD_TRAS_SAMP_RATE_16K,
	AUD_TRAS_SAMP_RATE_MAX,
} audio_tras_samp_rate_t;

typedef struct {
	uint16_t adc_gain;
	uint16_t dac_gain;
	uint16_t mic_samp_rate_points;		//the number of points in mic frame
	uint8_t mic_frame_number;			//the max frame number of mic ring buffer
	uint16_t speaker_samp_rate_points;	//the number of points in speaker frame
	uint8_t speaker_frame_number;		//the max frame number of speaker ring buffer
} aud_setup_t;

/* audio transfer driver setup config */
typedef struct {
	//audio_tras_drv_mode_t aud_trs_mode;
	audio_tras_samp_rate_t samp_rate;
	aud_setup_t aud_setup;
	bool aec_enable;
	aec_config_t *aec_setup;
	tx_context_t tx_context;
	rx_context_t rx_context;
	aud_cb_t aud_cb;
#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
	aec_dump_t aec_dump;
#endif
} aud_tras_drv_setup_t;

#ifdef __cplusplus
}
#endif
