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

#include <common/bk_err.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	PPI_320X240     = (1 << 0), /**< 320 * 240 */
	PPI_480X272     = (1 << 1), /**< 480 * 272 */
	PPI_640X480     = (1 << 2), /**< 640 * 480 */
	PPI_800X600     = (1 << 3), /**< 800 * 600 */
	PPI_1280X720    = (1 << 4), /**< 1280 * 720 */
} sensor_ppi_t;


typedef enum
{
	STATE_INVALID = 0,
	STATE_ALLOCED,
	STATE_FRAMED,
} frame_state_t;


typedef struct
{
	frame_state_t state;
	uint8_t id;
	uint8_t lock;
	uint8_t *frame;
	uint32_t length;
	uint32_t size;
	uint32_t sequence;
} frame_buffer_t;


static inline uint16_t ppi_to_pixel_x(sensor_ppi_t ppi)
{
	switch (ppi)
	{
		case PPI_320X240: return 320;
		case PPI_480X272: return 480;
		case PPI_640X480: return 640;
		case PPI_800X600: return 800;
		case PPI_1280X720: return 1280;
	}

	return 0;
}

static inline uint16_t ppi_to_pixel_y(sensor_ppi_t ppi)
{
	switch (ppi)
	{
		case PPI_320X240: return 240;
		case PPI_480X272: return 272;
		case PPI_640X480: return 480;
		case PPI_800X600: return 600;
		case PPI_1280X720: return 720;
	}

	return 0;
}


/*
 * @}
 */

#ifdef __cplusplus
}
#endif
