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
#define JPEG_CRC_SIZE (5)


#define FRAME_BUFFER_CACHE (1024 * 10)


typedef struct
{
	uint8 index;
	uint8 eof;
	uint8 psram_dma;
	uint8 psram_dma_busy;
	uint16 psram_dma_left;
	uint8 *buffer;
	frame_buffer_t *frame;
} dvp_camera_drv_t;

dvp_camera_drv_t *dvp_camera_drv = NULL;


const dvp_sensor_config_t *dvp_sensor_configs[] =
{
	&dvp_sensor_gc0328c,
	&dvp_sensor_hm1055,
	&dvp_sensor_ov2640,
};


static const dvp_sensor_config_t *current_sensor = NULL;
static const dvp_camera_config_t *dvp_camera_config = NULL;

static uint8_t dvp_camera_dma_channel = 0;

static uint8_t dvp_diag_debug = 0;

#define DVP_BUFFER_SIZE 1024
#define DVP_BUFFER_COUNT 4

uint32_t sequence = 0;
uint32_t media_jpg_isr_count = 0;


frame_buffer_t *curr_frame_buffer = NULL;

dvp_camera_device_t *dvp_camera_device = NULL;

bk_err_t dvp_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnls);

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

static void dvp_memcpy_finish_callback(dma_id_t id)
{
	if (dvp_diag_debug)
	{
		bk_gpio_pull_up(GPIO_2);
	}

	dvp_camera_drv->psram_dma_busy = false;
	dvp_camera_drv->index = !dvp_camera_drv->index;

	if (dvp_camera_drv->eof == true)
	{
		frame_buffer_t *frame = dvp_camera_drv->frame;

		if (dvp_camera_drv->psram_dma_left != 0)
		{
			dvp_memcpy_by_chnl(frame->frame + frame->length,
			                   dvp_camera_drv->index ? (dvp_camera_drv->buffer + FRAME_BUFFER_CACHE) : dvp_camera_drv->buffer,
			                   dvp_camera_drv->psram_dma_left, dvp_camera_drv->psram_dma);
			frame->length += dvp_camera_drv->psram_dma_left;
			frame->sequence = ++sequence;
			dvp_camera_drv->psram_dma_left = 0;
		}
		else
		{
			dvp_camera_config->frame_complete(frame);
			dvp_camera_drv->index = 0;
			dvp_camera_drv->frame = NULL;
			dvp_camera_drv->eof = false;

			curr_frame_buffer = dvp_camera_config->frame_alloc();

			if (curr_frame_buffer == NULL
			    || curr_frame_buffer->frame == NULL)
			{
				LOGE("alloc frame error\n");
				return;
			}

			curr_frame_buffer->length = 0;

			bk_dma_start(dvp_camera_dma_channel);
		}
	}


	if (dvp_diag_debug)
	{
		bk_gpio_pull_down(GPIO_2);
	}
}

bk_err_t dvp_memcpy_by_chnl(void *out, const void *in, uint32_t len, dma_id_t cpy_chnls)
{
	dma_config_t dma_config;

	/* fix for psram 4bytes alignment */
	if (len % 4)
	{
		len = (len / 4 + 1) * 4;
	}

	os_memset(&dma_config, 0, sizeof(dma_config_t));

	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 1;

	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.src.start_addr = (uint32_t)in;
	dma_config.src.end_addr = (uint32_t)(in + len);

	dma_config.dst.dev = DMA_DEV_DTCM;
	dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
	dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
	dma_config.dst.start_addr = (uint32_t)out;
	dma_config.dst.end_addr = (uint32_t)(out + len);

	dvp_camera_drv->psram_dma_busy = true;

	BK_LOG_ON_ERR(bk_dma_init(cpy_chnls, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(cpy_chnls, len));

	BK_LOG_ON_ERR(bk_dma_register_isr(cpy_chnls, NULL, dvp_memcpy_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(cpy_chnls));
	BK_LOG_ON_ERR(bk_dma_start(cpy_chnls));
	//BK_WHILE (bk_dma_get_enable_status(cpy_chnl));


	return BK_OK;

}


static void dvp_camera_eof_handler(jpeg_unit_t id, void *param)
{
	uint32_t real_length = bk_jpeg_enc_get_frame_size();
	uint32_t remain_length = FRAME_BUFFER_CACHE - bk_dma_get_remain_len(dvp_camera_dma_channel);
	uint32_t left_length = remain_length - JPEG_CRC_SIZE;

	media_jpg_isr_count++;

	if (dvp_diag_debug)
	{
		bk_gpio_pull_up(GPIO_8);
	}

	if (curr_frame_buffer == NULL
	    || curr_frame_buffer->frame == NULL)
	{
		LOGE("curr_frame_buffer NULL error\n");
		goto error;
	}

	if (curr_frame_buffer->length + left_length != real_length)
	{
		LOGW("size no match: %u:%u:%u\n", curr_frame_buffer->length + left_length, real_length, remain_length);
	}

	if (dvp_camera_drv->psram_dma_busy == true)
	{
		dvp_camera_drv->frame = curr_frame_buffer;
		dvp_camera_drv->psram_dma_left = left_length;
		dvp_camera_drv->eof = true;
	}
	else
	{
		dvp_camera_drv->frame = curr_frame_buffer;
		dvp_camera_drv->psram_dma_left = 0;
		dvp_camera_drv->eof = true;

		dvp_memcpy_by_chnl(curr_frame_buffer->frame + curr_frame_buffer->length,
		                   dvp_camera_drv->index ? (dvp_camera_drv->buffer + FRAME_BUFFER_CACHE) : dvp_camera_drv->buffer,
		                   left_length, dvp_camera_drv->psram_dma);
		curr_frame_buffer->length += left_length;
		curr_frame_buffer->sequence = ++sequence;
	}

	bk_dma_stop(dvp_camera_dma_channel);

#if 0
	dvp_camera_drv->index = 0;
	bk_dma_start(dvp_camera_dma_channel);
#endif

	if (dvp_diag_debug)
	{
		bk_gpio_pull_down(GPIO_8);
	}

	return;

error:
	bk_dma_stop(dvp_camera_dma_channel);
	bk_jpeg_enc_set_enable(0);
}

static void dvp_camera_dma_finish_callback(dma_id_t id)
{
	if (dvp_diag_debug)
	{
		bk_gpio_pull_up(GPIO_9);
	}

	if (curr_frame_buffer == NULL
	    || curr_frame_buffer->frame == NULL)
	{
		LOGE("%s curr_frame_buffer NULL\n");
		return;
	}

	dvp_memcpy_by_chnl(curr_frame_buffer->frame + curr_frame_buffer->length,
	                   dvp_camera_drv->index ? (dvp_camera_drv->buffer + FRAME_BUFFER_CACHE) : dvp_camera_drv->buffer,
	                   FRAME_BUFFER_CACHE, dvp_camera_drv->psram_dma);
	curr_frame_buffer->length += FRAME_BUFFER_CACHE;

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
	dma_config.dst.start_addr = (uint32_t)dvp_camera_drv->buffer;
	dma_config.dst.end_addr = (uint32_t)(dvp_camera_drv->buffer + FRAME_BUFFER_CACHE * 2);

	//os_printf("dst_start_addr:%08x, dst_end_addr:%08x\r\n", (uint32_t)info.buffer_addr, dma_config.dst.end_addr);

	BK_LOG_ON_ERR(bk_dma_init(dvp_camera_dma_channel, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dvp_camera_dma_channel, FRAME_BUFFER_CACHE));
	BK_LOG_ON_ERR(bk_dma_register_isr(dvp_camera_dma_channel, NULL, dvp_camera_dma_finish_callback));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dvp_camera_dma_channel));
	BK_LOG_ON_ERR(bk_dma_start(dvp_camera_dma_channel));

	return ret;
}

static void dvp_camera_yuv_eof_handler(jpeg_unit_t id, void *param)
{
	frame_buffer_t *frame = NULL;

	media_jpg_isr_count++;

	curr_frame_buffer->sequence = ++sequence;

	dvp_camera_config->frame_complete(curr_frame_buffer);

	frame = dvp_camera_config->frame_alloc();

	if (frame != NULL)
	{
		curr_frame_buffer = frame;
		bk_jpeg_set_em_base_addr(curr_frame_buffer->frame);
	}
	else
	{
		LOGE("%s malloc frame failed\n", __func__);
	}
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
		//gpio_dev_unmap(GPIO_2);
		//bk_gpio_pull_down(GPIO_2);
		//gpio_dev_unmap(GPIO_3);
		//bk_gpio_pull_down(GPIO_3);
		//gpio_dev_unmap(GPIO_4);
		//bk_gpio_pull_down(GPIO_4);
		//gpio_dev_unmap(GPIO_5);
		//bk_gpio_pull_down(GPIO_5);
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

bk_err_t bk_dvp_camera_driver_init(const dvp_camera_config_t *config, dvp_mode_t mode)
{
	int ret = kNoErr;
	i2c_config_t i2c_config = {0};
	jpeg_config_t jpeg_config = {0};

	BK_ASSERT(dvp_camera_config->frame_alloc != NULL);
	BK_ASSERT(dvp_camera_config->frame_complete != NULL);

#if (CONFIG_PSRAM)
		bk_psram_init();
#endif

	if (dvp_camera_device == NULL)
	{
		dvp_camera_device = (dvp_camera_device_t *)os_malloc(sizeof(dvp_camera_device_t));

		if (dvp_camera_device == NULL)
		{
			LOGE("dvp_camera_device malloc failed\n");
			goto error;
		}
	
		os_memset(dvp_camera_device, 0, sizeof(dvp_camera_device_t));
	}

	dvp_camera_device->mode = mode;

	dvp_camera_config = config;

	bk_dvp_camera_gpio_init(dvp_camera_config, 1);


	ret = bk_jpeg_enc_driver_init();

	if (ret != BK_OK)
	{
		LOGE("jpeg encode driver init failed\n");
		goto error;
	}

	bk_jpeg_enc_mclk_enable();

	if (mode == DVP_MODE_JPG)
	{
		jpeg_config.yuv_mode = 0;
	}
	else if (mode == DVP_MODE_YUV)
	{
		jpeg_config.yuv_mode = 1;
	}

	i2c_config.baud_rate = dvp_camera_config->host->baud_rate;
	i2c_config.addr_mode = dvp_camera_config->host->addr_mode;
	bk_i2c_init(dvp_camera_config->host->id, &i2c_config);

	current_sensor = get_sensor_auto_detect(dvp_camera_config);

	if (current_sensor == NULL)
	{
		LOGE("%s no dvp camera found\n", __func__);
		ret = kGenericErrorBase;
		goto error;
	}

	if (mode == DVP_MODE_JPG)
	{

		if (dvp_camera_drv == NULL)
		{
			dvp_camera_drv = (dvp_camera_drv_t *)os_malloc(sizeof(dvp_camera_drv_t));

			if (dvp_camera_drv == NULL)
			{
				LOGE("dvp_camera_drv malloc failed\n");
				goto error;
			}

			os_memset(dvp_camera_drv, 0, sizeof(dvp_camera_drv_t));

			dvp_camera_drv->buffer = (uint8 *)os_malloc(FRAME_BUFFER_CACHE * 2);

			if (dvp_camera_drv->buffer == NULL)
			{
				LOGE("dvp_camera_drv malloc failed\n");
				goto error;
			}

		}

		ret = dvp_camera_dma_config();

		if (ret != BK_OK)
		{
			LOGE("dma init failed\n");
			goto error;
		}

		dvp_camera_drv->psram_dma = bk_dma_alloc(DMA_DEV_JPEG);
		if ((dvp_camera_drv->psram_dma < DMA_ID_0) || (dvp_camera_drv->psram_dma >= DMA_ID_MAX))
		{
			LOGE("malloc dvp_camera_drv->psram_dma fail \r\n");
			ret = BK_FAIL;
			goto error;
		}

	}

	switch (current_sensor->def_ppi)
	{
		case PPI_320X240:
		case PPI_480X272:
		case PPI_640X480:
		case PPI_800X600:
		case PPI_1280X720:
		case PPI_1600X1200:
		{
			jpeg_config.x_pixel = ppi_to_pixel_x(current_sensor->def_ppi) / 8;
			jpeg_config.y_pixel = ppi_to_pixel_y(current_sensor->def_ppi) / 8;
		}

		default:
			break;
	}

	switch (current_sensor->clk)
	{
		case JPEG_96M_MCLK_16M:
			jpeg_config.sys_clk_div = 4;
			jpeg_config.mclk_div = 1;
			break;

		case JPEG_96M_MCLK_24M:
			jpeg_config.sys_clk_div = 4;
			jpeg_config.mclk_div = 0;
			break;

		case JPEG_120M_MCLK_30M:
			jpeg_config.sys_clk_div = 3;
			jpeg_config.mclk_div = 0;
			break;

		default:
			break;
	}


	ret = bk_jpeg_enc_dvp_init(&jpeg_config);
	if (ret != BK_OK)
	{
		LOGE("jpeg init error\n");
		goto error;
	}

	if (mode == DVP_MODE_JPG)
	{
		bk_jpeg_enc_register_isr(END_OF_FRAME, dvp_camera_eof_handler, NULL);
	}
	else if (mode == DVP_MODE_YUV)
	{
		curr_frame_buffer = dvp_camera_config->frame_alloc();
		
		if (curr_frame_buffer == NULL)
		{
			LOGE("malloc frame fail \r\n");
			ret = BK_FAIL;
			goto error;
		}

		bk_jpeg_enc_register_isr(END_OF_YUV, dvp_camera_yuv_eof_handler, NULL);
		bk_jpeg_set_em_base_addr(curr_frame_buffer->frame);
	}


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

	media_jpg_isr_count = 0;

	LOGI("dvp camera init complete with mode: %d\n", mode);

	return ret;

error:

	if (dvp_camera_drv != NULL)
	{
		if (dvp_camera_drv->buffer != NULL)
		{
			os_free(dvp_camera_drv->buffer);
			dvp_camera_drv->buffer = NULL;
		}

		os_free(dvp_camera_drv);
		dvp_camera_drv = NULL;
	}

	if (dvp_camera_device)
	{
		os_free(dvp_camera_device);
		dvp_camera_device = NULL;
	}

	LOGI("dvp camera init failed\n");

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

	if (dvp_camera_drv)
	{
		bk_dma_stop(dvp_camera_drv->psram_dma);
		bk_dma_deinit(dvp_camera_drv->psram_dma);
		bk_dma_free(DMA_DEV_JPEG, dvp_camera_drv->psram_dma);
		os_free(dvp_camera_drv->buffer);
		os_free(dvp_camera_drv);
		dvp_camera_drv = NULL;
	}

	bk_i2c_deinit(dvp_camera_config->host->id);

#if CONFIG_SOC_BK7256XX
	bk_jpeg_enc_driver_deinit();
#endif

	if (dvp_camera_device)
	{
		os_free(dvp_camera_device);
		dvp_camera_device = NULL;
	}

	current_sensor = NULL;
	dvp_camera_config = NULL;
	dvp_camera_dma_channel = 0;
	curr_frame_buffer = NULL;

	LOGI("dvp camera deinit complete\n");
	return kNoErr;
}

