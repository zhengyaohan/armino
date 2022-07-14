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

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>

#include <driver/media_types.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>
#include "dvp_sensor_devices.h"

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "gpio_driver.h"
#include <driver/gpio.h>

#if (CONFIG_PSRAM)
#include <driver/psram.h>
#endif

#define TAG "dvp_drv"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define FRAME_BUFFER_DMA_TH (1024 * 10)


const dvp_sensor_config_t *dvp_sensor_configs[] =
{
	&dvp_sensor_gc0328c,
	&dvp_sensor_hm1055,
};


static const dvp_sensor_config_t *current_sensor = NULL;
static const dvp_camera_config_t *dvp_camera_config = NULL;

static uint8_t  dvp_camera_dma_channel = 0;

static uint8_t dvp_diag_debug = 1;

#define DVP_BUFFER_SIZE 1024
#define DVP_BUFFER_COUNT 4

uint32_t sequence = 0;

frame_buffer_t *curr_frame_buffer = NULL;

dvp_camera_device_t *dvp_camera_device = NULL;

const dvp_sensor_config_t *get_sensor_config_interface_by_id(sensor_id_t id)
{
	uint32_t i, size = sizeof(dvp_sensor_configs);

	for (i = 0; i < size; i++)
	{
		if (dvp_sensor_configs[i]->id == id)
		{
			return dvp_sensor_configs[i];
		}
	}

	return NULL;
}

//uint8_t state_flag = 0;


static void dvp_camera_eof_handler(jpeg_unit_t id, void *param)
{
	if (dvp_diag_debug)
	{
		bk_gpio_pull_up(GPIO_8);
	}

	//if (state_flag)
	//{
	//  return;
	//}

	if (curr_frame_buffer != NULL
	    && curr_frame_buffer->frame != NULL)
	{
		curr_frame_buffer->length += FRAME_BUFFER_DMA_TH - bk_dma_get_remain_len(dvp_camera_dma_channel) - 5;
		curr_frame_buffer->sequence = ++sequence;
	}

	dvp_camera_config->frame_complete(curr_frame_buffer);

	//if (!state_flag)
	//{
	//  bk_dma_stop(dvp_camera_dma_channel);
	//  bk_jpeg_enc_set_enable(0);
	//  state_flag = 1;
	//}


	curr_frame_buffer = dvp_camera_config->frame_alloc();

	if (curr_frame_buffer != NULL
	    && curr_frame_buffer->frame != NULL)
	{
		bk_dma_stop(dvp_camera_dma_channel);

		curr_frame_buffer->length = 0;
		BK_LOG_ON_ERR(bk_dma_set_dest_addr(dvp_camera_dma_channel, (uint32_t)curr_frame_buffer->frame, (uint32_t)(curr_frame_buffer->frame + curr_frame_buffer->size)));
		bk_dma_start(dvp_camera_dma_channel);
	}

	if (dvp_diag_debug)
	{
		bk_gpio_pull_down(GPIO_8);
	}
}

#if 0
static void yuv_eof_handle(jpeg_unit_t id, void *param)
{
	if (dvp_diag_debug)
	{
		bk_gpio_pull_up(GPIO_4);
	}

	bk_jpeg_enc_set_enable(0);
	bk_jpeg_enc_set_yuv_config();
	BK_LOG_ON_ERR(bk_dma_start(dvp_camera_dma_channel));


	if (dvp_diag_debug)
	{
		bk_gpio_pull_down(GPIO_4);
	}
}
#endif

static void dvp_camera_dma_finish_callback(dma_id_t id)
{
	if (dvp_diag_debug)
	{
		bk_gpio_pull_up(GPIO_9);
	}

	if (curr_frame_buffer != NULL
	    && curr_frame_buffer->frame != NULL)
	{
		curr_frame_buffer->length += FRAME_BUFFER_DMA_TH;
	}

	//TODO

	if (dvp_diag_debug)
	{
		bk_gpio_pull_down(GPIO_9);
	}
}

static bk_err_t dvp_camera_dma_config(void)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_config = {0};
	uint32_t jpeg_fifo_addr;

	curr_frame_buffer = dvp_camera_config->frame_alloc();

	if (curr_frame_buffer == NULL)
	{
		LOGE("malloc frame fail \r\n");
		ret = BK_FAIL;
		return ret;
	}


	curr_frame_buffer->length = 0;

	dvp_camera_dma_channel = bk_dma_alloc(DMA_DEV_JPEG);
	if ((dvp_camera_dma_channel < DMA_ID_0) || (dvp_camera_dma_channel >= DMA_ID_MAX))
	{
		LOGE("malloc dma fail \r\n");
		ret = BK_FAIL;
		return ret;
	}
	bk_jpeg_enc_get_fifo_addr(&jpeg_fifo_addr);

	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_JPEG;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = jpeg_fifo_addr;
	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
	dma_config.dst.start_addr = (uint32_t)curr_frame_buffer->frame;
	dma_config.dst.end_addr = (uint32_t)(curr_frame_buffer->frame + curr_frame_buffer->size);

	//os_printf("dst_start_addr:%08x, dst_end_addr:%08x\r\n", (uint32_t)info.buffer_addr, dma_config.dst.end_addr);

	BK_LOG_ON_ERR(bk_dma_init(dvp_camera_dma_channel, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dvp_camera_dma_channel, FRAME_BUFFER_DMA_TH));
	BK_LOG_ON_ERR(bk_dma_register_isr(dvp_camera_dma_channel, NULL, dvp_camera_dma_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dvp_camera_dma_channel));
	BK_LOG_ON_ERR(bk_dma_start(dvp_camera_dma_channel));

	return ret;
}


const dvp_sensor_config_t *get_sensor_auto_detect(const dvp_camera_config_t *config)
{
	uint32_t i, size = sizeof(dvp_sensor_configs) / sizeof(const dvp_sensor_config_t *);

	for (i = 0; i < size; i++)
	{
		if (dvp_sensor_configs[i]->detect(config) == true)
		{
			return dvp_sensor_configs[i];
		}
	}

	return NULL;
}


bk_err_t bk_dvp_camera_gpio_init(const dvp_camera_config_t *config, uint8_t mode)
{

	if (mode == 1)
	{

		gpio_dev_unmap(config->host->mclk);
		gpio_dev_map(config->host->mclk, GPIO_DEV_JPEG_MCLK);

		gpio_dev_unmap(config->host->pck);
		gpio_dev_map(config->host->pck, GPIO_DEV_JPEG_PCLK);

	}

	if (mode == 2)
	{
		gpio_dev_unmap(config->host->hsync);
		gpio_dev_map(config->host->hsync, GPIO_DEV_JPEG_HSYNC);

		gpio_dev_unmap(config->host->vsync);
		gpio_dev_map(config->host->vsync, GPIO_DEV_JPEG_VSYNC);

		gpio_dev_unmap(config->host->pxdata0);
		gpio_dev_map(config->host->pxdata0, GPIO_DEV_JPEG_PXDATA0);

		gpio_dev_unmap(config->host->pxdata1);
		gpio_dev_map(config->host->pxdata1, GPIO_DEV_JPEG_PXDATA1);

		gpio_dev_unmap(config->host->pxdata2);
		gpio_dev_map(config->host->pxdata2, GPIO_DEV_JPEG_PXDATA2);

		gpio_dev_unmap(config->host->pxdata3);
		gpio_dev_map(config->host->pxdata3, GPIO_DEV_JPEG_PXDATA3);

		gpio_dev_unmap(config->host->pxdata4);
		gpio_dev_map(config->host->pxdata4, GPIO_DEV_JPEG_PXDATA4);

		gpio_dev_unmap(config->host->pxdata5);
		gpio_dev_map(config->host->pxdata5, GPIO_DEV_JPEG_PXDATA5);

		gpio_dev_unmap(config->host->pxdata6);
		gpio_dev_map(config->host->pxdata6, GPIO_DEV_JPEG_PXDATA6);

		gpio_dev_unmap(config->host->pxdata7);
		gpio_dev_map(config->host->pxdata7, GPIO_DEV_JPEG_PXDATA7);
	}
	if (dvp_diag_debug)
	{
		gpio_dev_unmap(GPIO_2);
		bk_gpio_pull_down(GPIO_2);
		gpio_dev_unmap(GPIO_3);
		bk_gpio_pull_down(GPIO_3);
		gpio_dev_unmap(GPIO_4);
		//bk_gpio_pull_down(GPIO_4);
		gpio_dev_unmap(GPIO_5);
		bk_gpio_pull_down(GPIO_5);
		gpio_dev_unmap(GPIO_6);
		bk_gpio_pull_down(GPIO_6);
		gpio_dev_unmap(GPIO_7);
		bk_gpio_pull_down(GPIO_7);
		gpio_dev_unmap(GPIO_8);
		bk_gpio_pull_down(GPIO_8);
		gpio_dev_unmap(GPIO_9);
		bk_gpio_pull_down(GPIO_9);
	}

	return 0;
}

bk_err_t bk_dvp_camera_driver_init(const dvp_camera_config_t *config)
{
	int ret = kNoErr;
	i2c_config_t i2c_config = {0};
	jpeg_config_t jpeg_config = {0};

	BK_ASSERT(dvp_camera_config->frame_alloc != NULL);
	BK_ASSERT(dvp_camera_config->frame_complete != NULL);

#if (CONFIG_PSRAM)
	bk_psram_init(0x00054043);
#endif

	dvp_camera_device = (dvp_camera_device_t*)os_malloc(sizeof(dvp_camera_device_t));
	os_memset(dvp_camera_device, 0, sizeof(dvp_camera_device_t));

	dvp_camera_config = config;

	bk_dvp_camera_gpio_init(dvp_camera_config, 1);

	extern bk_err_t bk_clk_enable(void);

	// step 2: jpeg enc driver init
	ret = bk_jpeg_enc_driver_init();
	if (ret != BK_OK)
	{
		os_printf("jpeg encode driver init failed\r\n");
		return ret;
	}

	bk_clk_enable();
	jpeg_config.yuv_mode = 0;

	i2c_config.baud_rate = dvp_camera_config->host->baud_rate;
	i2c_config.addr_mode = dvp_camera_config->host->addr_mode;
	bk_i2c_init(dvp_camera_config->host->id, &i2c_config);

	current_sensor = get_sensor_auto_detect(dvp_camera_config);

	if (current_sensor == NULL)
	{
		LOGE("%s no dvp camera found\n", __func__);
		return kGenericErrorBase;
	}

	// step 3: dma_init
	ret = dvp_camera_dma_config();
	if (ret != BK_OK)
	{
		LOGE("dma init failed\r\n");
		return ret;
	}

	switch (current_sensor->def_ppi)
	{
		case PPI_320X240:
		{
			jpeg_config.x_pixel = X_PIXEL_320;
			jpeg_config.y_pixel = Y_PIXEL_480;
		}
		break;

		case PPI_480X272:
		{
			jpeg_config.x_pixel = X_PIXEL_480;
			jpeg_config.y_pixel = Y_PIXEL_272;
		}
		break;
		case PPI_640X480:
		{
			jpeg_config.x_pixel = X_PIXEL_640;
			jpeg_config.y_pixel = Y_PIXEL_480;
		}
		break;

		case PPI_800X600:
		case PPI_1280X720:
		default:
			break;

	}


	if (jpeg_config.y_pixel == Y_PIXEL_720)
	{
		jpeg_config.sys_clk_div = 3;
		jpeg_config.mclk_div = 2;
	}
	else
	{
		jpeg_config.sys_clk_div = 4;
		jpeg_config.mclk_div = 0;
	}

	ret = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (ret != BK_OK)
	{
		LOGE("jpeg init error\n");
		return ret;
	}

	bk_jpeg_enc_register_isr(END_OF_FRAME, dvp_camera_eof_handler, NULL);
	//bk_jpeg_enc_register_isr(END_OF_YUV, yuv_eof_handle, NULL);

	current_sensor->init(dvp_camera_config);
	bk_dvp_camera_gpio_init(dvp_camera_config, 2);
	current_sensor->set_fps(dvp_camera_config, current_sensor->def_fps);
	current_sensor->set_ppi(dvp_camera_config, current_sensor->def_ppi);

	dvp_camera_device->ppi = current_sensor->def_ppi;
	dvp_camera_device->fps = current_sensor->def_fps;
	dvp_camera_device->name = current_sensor->name;
	dvp_camera_device->id = current_sensor->id;
	dvp_camera_device->fps_cap = current_sensor->fps_cap;
	dvp_camera_device->ppi_cap = current_sensor->ppi_cap;

	LOGI("dvp camera init complete\n");

	return ret;
}


dvp_camera_device_t *bk_dvp_camera_get_device(void)
{
	return dvp_camera_device;
}

bk_err_t bk_dvp_camera_driver_deinit(void)
{

	bk_jpeg_enc_deinit();

	bk_dma_stop(dvp_camera_dma_channel);

	bk_dma_deinit(dvp_camera_dma_channel);

	bk_dma_free(DMA_DEV_JPEG, dvp_camera_dma_channel);

	bk_i2c_deinit(dvp_camera_config->host->id);

#if CONFIG_SOC_BK7256XX
	bk_jpeg_enc_driver_deinit();
#endif

	current_sensor = NULL;
	dvp_camera_config = NULL;
	dvp_camera_dma_channel = 0;
	curr_frame_buffer = NULL;

	LOGI("dvp camera deinit complete\n");
	return kNoErr;
}

