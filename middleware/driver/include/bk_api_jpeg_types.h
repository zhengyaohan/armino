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

#include "bk_include.h"
#include "jpeg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Y_PIXEL_272     (34)  // Y * 8
#define X_PIXEL_480     (60)  // X * 8
#define Y_PIXEL_480     (60)  // Y * 8
#define X_PIXEL_640     (80)  // X * 8
#define Y_PIXEL_240     (30)  // Y * 8
#define X_PIXEL_320     (40)  // X * 8
#define Y_PIXEL_600     (75)  // Y * 8
#define X_PIXEL_800     (100) // X * 8
#define Y_PIXEL_720     (90)  // Y * 8
#define X_PIXEL_1280    (160) // X * 8

typedef void (*jpeg_isr_t)(jpeg_unit_t id, void *param);

typedef struct ejpeg_desc {
	uint8_t *rxbuf;

	void (*node_full_handler)(void *curptr, uint32_t newlen, uint32_t is_eof, uint32_t frame_len);
	void (*data_end_handler)(void);

	uint16_t rxbuf_len;
	uint16_t rx_read_len;
	uint32_t node_len;
	uint32_t sener_cfg;
	uint16_t x_pixel;
	uint16_t y_pixel;

	jpeg_isr_t start_frame_handler;
	jpeg_isr_t end_frame_handler;
	jpeg_isr_t end_yuv_handler;

#if CONFIG_GENERAL_DMA
	dma_isr_t dma_rx_handler;
	uint32_t dma_channel;
#endif
} DJPEG_DESC_ST;

#ifdef __cplusplus
}
#endif

