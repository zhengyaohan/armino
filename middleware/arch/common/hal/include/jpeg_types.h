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

#include "bk_api_dma_types.h"
#include "bk_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_JPEG_NOT_INIT    (BK_ERR_JPEG_BASE - 1) /**< JPEG driver not init */

typedef uint8_t jpeg_unit_t; /**< jpeg uint id */

typedef struct {
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
	uint32_t node_len;
	uint32_t x_pixel;
	uint32_t y_pixel;
	dma_isr_t dma_rx_finish_handler;
} jpeg_config_t;

#ifdef __cplusplus
}
#endif

