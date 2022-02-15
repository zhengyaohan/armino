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

#include "arch_interrupt.h"
#include "bk_api_int_types.h"
#include "bk_include.h"
#include "dma_types.h"
#include "spi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART interrupt service routine
 */

typedef void (*spi_isr_t)(spi_id_t id, void *param);

/**
 * @}
 */

typedef struct {
	spi_id_t id;
	icu_int_src_t int_src;
	int_group_isr_t isr;
	dma_dev_t dma_dev;
} spi_int_config_t;

#ifdef __cplusplus
}
#endif

