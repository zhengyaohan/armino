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
#include <components/log.h>

#include "media_cli_comm.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include "dvp_api.h"


#define TAG "mcli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

static const dvp_host_config_t host_config =
{
	.id = CONFIG_CAMERA_I2C_ID,
	.baud_rate = I2C_BAUD_RATE_100KHZ,
	.addr_mode = I2C_ADDR_MODE_7BIT,
	.mclk = GPIO_27,
	.pck = GPIO_29,
	.hsync = GPIO_30,
	.vsync = GPIO_31,
	.pxdata0 = GPIO_32,
	.pxdata1 = GPIO_33,
	.pxdata2 = GPIO_34,
	.pxdata3 = GPIO_35,
	.pxdata4 = GPIO_36,
	.pxdata5 = GPIO_37,
	.pxdata6 = GPIO_38,
	.pxdata7 = GPIO_39
};

static void dvp_rx_handler(void *curptr, uint32_t newlen, uint32_t is_eof, uint32_t frame_len)
{

}

static void dvp_act_frame_complete_handler(frame_buffer_t *buffer)
{
	//TODO
}

static void dvp_eof_frame_complete_handler(void)
{
	//TODO
}


static const jpegenc_desc_t jpegenc_desc =
{
	.rxbuf_len = 1472 * 4,
	.node_len = 1472,
	.rx_read_len = 0,
	.sener_cfg = 0,
	.node_full_handler = dvp_rx_handler,
	.data_end_handler = dvp_eof_frame_complete_handler,
};

static const dvp_camera_config_t dvp_camera_config =
{
	.host = &host_config,
	.jpegenc_desc = &jpegenc_desc,
	.frame_complete = dvp_act_frame_complete_handler
};



void media_cli_dvp_open(void)
{
	dvp_open(&dvp_camera_config);
}
