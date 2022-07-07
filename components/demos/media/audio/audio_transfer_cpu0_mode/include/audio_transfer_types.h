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

#ifdef __cplusplus
extern "C" {
#endif

#define AUD_AEC_8K_FRAME_SAMP_SIZE           160*2

/* G711 decoder 每帧数据为一个字节，所以可以设置很小来节约memory，但是从系统task调度考虑，不易太小，*/
#define AUD_DECD_FRAME_SAMP_SIZE             320


/* used in cpu1 */
typedef enum {
	AUDIO_SAMP_RATE_8K = 0,
	AUDIO_SAMP_RATE_16K,
	AUDIO_SAMP_RATE_MAX,
} audio_sample_rate_t;

typedef enum {
	AUDIO_TRANSFER_IDLE = 0,
	AUDIO_TRANSFER_AEC,
	AUDIO_TRANSFER_ENCODER,
	AUDIO_TRANSFER_DECODER,
	AUDIO_TRANSFER_START,
	AUDIO_TRANSFER_EXIT,
	AUDIO_TRANSFER_MAX,
} audio_transfer_opcode_t;

typedef struct {
	audio_transfer_opcode_t op;
} audio_transfer_msg_t;

typedef struct {
	audio_sample_rate_t samp_rate;
} audio_setup_t;

#ifdef __cplusplus
}
#endif
