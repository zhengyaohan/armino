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
#include "driver/dvp_camera_types.h"
#include <driver/i2c.h>


#ifdef __cplusplus
extern "C" {
#endif



typedef struct
{
	i2c_id_t id;
	uint32_t baud_rate;
	i2c_addr_mode_t addr_mode;
	gpio_id_t mclk;
	gpio_id_t pck;
	gpio_id_t hsync;
	gpio_id_t vsync;
	gpio_id_t pxdata0;
	gpio_id_t pxdata1;
	gpio_id_t pxdata2;
	gpio_id_t pxdata3;
	gpio_id_t pxdata4;
	gpio_id_t pxdata5;
	gpio_id_t pxdata6;
	gpio_id_t pxdata7;
} dvp_host_config_t;


typedef struct
{
	const dvp_host_config_t *host;
	void (*frame_complete)(frame_buffer_t* buffer);
	frame_buffer_t* (*frame_alloc)(void);
} dvp_camera_config_t;



typedef struct
{
	char *name;
	media_ppi_t def_ppi;
	sensor_fps_t def_fps;
	uint16 id;
	uint8 clk;
	uint16 address;
	uint16 fps_cap;
	uint16 ppi_cap;
	bool (*detect)(const dvp_camera_config_t *config);
	int (*init)(const dvp_camera_config_t *config);
	int (*set_ppi)(const dvp_camera_config_t *config, media_ppi_t ppi);
	int (*set_fps)(const dvp_camera_config_t *config, sensor_fps_t fps);
} dvp_sensor_config_t;

#define GC_QVGA_USE_SUBSAMPLE          1

typedef struct
{
	media_ppi_t ppi;
	sensor_fps_t fps;
	char *name;
	uint16 id;
	uint16 fps_cap;
	uint16 ppi_cap;
	dvp_mode_t mode;
} dvp_camera_device_t;


bk_err_t bk_dvp_camera_driver_init(const dvp_camera_config_t *config, dvp_mode_t mode);
bk_err_t bk_dvp_camera_driver_deinit(void);
dvp_camera_device_t *bk_dvp_camera_get_device(void);


#ifdef __cplusplus
}
#endif
