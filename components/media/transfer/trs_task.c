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
#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>

#include <components/video_transfer.h>

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>

#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <driver/i2c.h>
#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>


#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>
#include <driver/media_types.h>

#include <soc/mapping.h>

#include "bk_general_dma.h"


#include "media_core.h"
#include "aud_act.h"
#include "adc_ccb.h"
#include "dac_ccb.h"
#include "trs_api.h"
#include "trs_act.h"
#include "trs_task.h"
#include "dvp_api.h"
#include "lcd_api.h"

#define TAG "trs_task"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define MAX_TX_SIZE (1472)
//#define MAX_COPY_SIZE (1472 - sizeof(transfer_data_t))
#define MAX_COPY_SIZE (1468)
#define MAX_RETRY (100)
#define RETRANSMITS_TIME (5)

static beken_queue_t trs_task_queue = NULL;
static beken_thread_t trs_task_thread = NULL;


frame_buffer_t dvp_frame[3] = {0};

transfer_data_t *wifi_tranfer_data = NULL;

uint8_t frame_id = 0;


video_setup_t vido_transfer_info = {0};

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

bk_err_t trs_task_send_msg(uint8_t msg_type, uint32_t data);


static frame_buffer_t *dvp_frame_alloc_frame(void)
{
	frame_buffer_t *buffer = NULL;
	uint32_t i;

	for (i = 0; i < sizeof(dvp_frame) / sizeof(frame_buffer_t); i++)
	{
		if (dvp_frame[i].state != STATE_FRAMED)
		{
			continue;
		}

		if (buffer == NULL)
		{
			buffer = &dvp_frame[i];
			continue;
		}

		if (buffer->sequence < dvp_frame[i].sequence)
		{
			buffer = &dvp_frame[i];
		}
	}

	if (buffer != NULL)
	{
		buffer->state = STATE_ALLOCED;
	}

	return buffer;
}

static void dvp_frame_free_frame(frame_buffer_t *buffer)
{
	buffer->state = STATE_FRAMED;
}


static void dvp_rx_handler(void *curptr, uint32_t newlen, uint32_t is_eof, uint32_t frame_len)
{

}

static void dvp_act_eof_handler(void)
{
	//TODO
}



static void dvp_frame_complete(frame_buffer_t *buffer)
{
	dvp_frame_free_frame(buffer);
	//LOGI("id: %u, seq: %u, length: %u, size: %u\n", buffer->id, buffer->sequence, buffer->length, buffer->size);

	if (NULL == get_trs_transfer_lock_frame())
	{
		media_msg_t msg;

		frame_buffer_t *frame = dvp_frame_alloc_frame();
		msg.event = EVENT_TRS_FRAME_COMPLETE_IND;
		msg.param = (uint32_t)frame;
		media_send_msg(&msg);
	}
}

static frame_buffer_t *dvp_frame_alloc_buffer(void)
{
	frame_buffer_t *buffer = NULL;
	uint32_t i;

	for (i = 0; i < sizeof(dvp_frame) / sizeof(frame_buffer_t); i++)
	{
		if (dvp_frame[i].state == STATE_ALLOCED)
		{
			continue;
		}

		if (buffer == NULL)
		{
			buffer = &dvp_frame[i];
			continue;
		}

		if (buffer->sequence > dvp_frame[i].sequence)
		{
			buffer = &dvp_frame[i];
		}
	}

	if (buffer != NULL)
	{
		buffer->state = STATE_ALLOCED;
	}

	//LOGI("id: %d\n", buffer->id);

	return buffer;
}



static const jpegenc_desc_t jpegenc_desc =
{
	.rxbuf_len = 1472 * 4,
	.node_len = 1472,
	.rx_read_len = 0,
	.sener_cfg = 0,
	.node_full_handler = dvp_rx_handler,
	.data_end_handler = dvp_act_eof_handler,
};

static const dvp_camera_config_t dvp_camera_config =
{
	.host = &host_config,
	.jpegenc_desc = &jpegenc_desc,
	.frame_complete = dvp_frame_complete,
	.frame_alloc = dvp_frame_alloc_buffer,
};

int dvp_frame_send(uint8_t *data, uint32_t size, uint32_t retry_max, uint32_t ms_time)
{
	int ret = kGeneralErr;

	if (!vido_transfer_info.send_func)
	{
		return ret;
	}

	do
	{
		ret = vido_transfer_info.send_func(data, size);

		if (ret == size)
		{
			//LOGI("size: %d\n", size);
			break;
		}

		//LOGI("retry\n");
		rtos_delay_milliseconds(ms_time);

	}
	while (retry_max--);


	return ret == size ? kNoErr : kGeneralErr;
}

static void dvp_frame_handle(frame_buffer_t *buffer)
{
	uint32_t i;
	uint32_t count = buffer->length / MAX_COPY_SIZE;
	uint32_t tail = buffer->length % MAX_COPY_SIZE;
	uint8_t id = frame_id++;
	int ret;
	media_msg_t msg;

	LOGD("id: %u, seq: %u, length: %u, size: %u\n", buffer->id, buffer->sequence, buffer->length, buffer->size);
	wifi_tranfer_data->id = id;
	wifi_tranfer_data->size = 0;
	wifi_tranfer_data->eof = 0;
	wifi_tranfer_data->cnt = 0;

	for (i = 0; i < count; i++)
	{
		if ((tail == 0) && (i == count - 1))
		{
			wifi_tranfer_data->eof = 1;
			wifi_tranfer_data->cnt = count;
		}

		dma_memcpy(wifi_tranfer_data->data, buffer->frame + (MAX_COPY_SIZE * i), MAX_COPY_SIZE);

		ret = dvp_frame_send((uint8_t *)wifi_tranfer_data, MAX_TX_SIZE, MAX_RETRY, RETRANSMITS_TIME);

		if (ret != kNoErr)
		{
			LOGE("send failed\n");
		}
	}

	if (tail)
	{
		wifi_tranfer_data->eof = 1;
		wifi_tranfer_data->cnt = count + 1;

		dma_memcpy(wifi_tranfer_data->data, buffer->frame + (MAX_COPY_SIZE * i), tail);

		ret = dvp_frame_send((uint8_t *)wifi_tranfer_data, tail + sizeof(transfer_data_t), MAX_RETRY, RETRANSMITS_TIME);

		if (ret != kNoErr)
		{
			LOGE("send failed\n");
		}
	}

	LOGD("seq: %u, length: %u, tail: %u, count: %u\n", id, buffer->length, tail, count);

	dvp_frame_free_frame(buffer);


	msg.event = EVENT_TRS_FRAME_FREE_IND;
	msg.param = (uint32_t)buffer;
	media_send_msg(&msg);
}


static void trs_task_entry(beken_thread_arg_t data)
{
	bk_err_t ret = BK_OK;
	trs_task_msg_t msg;

	dvp_open(&dvp_camera_config);
#ifdef CONFIG_LCD
	lcd_open();
	video_transfer_frame_register(MODULE_LCD);
#endif

	while (1)
	{
		ret = rtos_pop_from_queue(&trs_task_queue, &msg, BEKEN_WAIT_FOREVER);

		if (kNoErr == ret)
		{
			switch (msg.type)
			{
				case TRS_TRANSFER_DATA:
					dvp_frame_handle((frame_buffer_t *)msg.data);
					break;

				case TRS_TRANSFER_EXIT:
					goto exit;
				default:
					break;
			}
		}
	}

exit:
	dvp_close();

	if (wifi_tranfer_data != NULL) {
		os_free(wifi_tranfer_data);
		wifi_tranfer_data = NULL;
	}

	frame_id = 0;

	os_memset(dvp_frame, 0, sizeof(frame_buffer_t) * 3);

	rtos_deinit_queue(&trs_task_queue);
	trs_task_queue = NULL;

	trs_task_thread = NULL;
	rtos_delete_thread(NULL);
}

int trs_task_start(video_setup_t *setup_cfg)
{
	int ret;

	os_memcpy(&vido_transfer_info, setup_cfg, sizeof(video_setup_t));

	os_memset(dvp_frame, 0, sizeof(dvp_frame));

	dvp_frame[0].state = STATE_INVALID;
	dvp_frame[0].frame = psram_map->jpeg_enc[0];
	dvp_frame[0].size = sizeof(psram_map->jpeg_enc[0]);
	dvp_frame[0].id = 0;

	dvp_frame[1].state = STATE_INVALID;
	dvp_frame[1].frame = psram_map->jpeg_enc[1];
	dvp_frame[1].size = sizeof(psram_map->jpeg_enc[1]);
	dvp_frame[1].id = 1;

	dvp_frame[2].state = STATE_INVALID;
	dvp_frame[2].frame = psram_map->jpeg_enc[2];
	dvp_frame[2].size = sizeof(psram_map->jpeg_enc[2]);
	dvp_frame[2].id = 2;

	if (wifi_tranfer_data == NULL)
	{
		wifi_tranfer_data = (transfer_data_t *) os_malloc(MAX_TX_SIZE);
	}

	frame_id = 0;

	if (trs_task_queue == NULL)
	{
		ret = rtos_init_queue(&trs_task_queue, "trs_task_queue", sizeof(trs_task_msg_t), 60);

		if (kNoErr != ret)
		{
			LOGE("%s trs_task_queue init failed\n");
			goto error;
		}
	}

	if (trs_task_thread == NULL)
	{
		ret = rtos_create_thread(&trs_task_thread,
		                         4,
		                         "trs_task_thread",
		                         (beken_thread_function_t)trs_task_entry,
		                         4 * 1024,
		                         NULL);

		if (kNoErr != ret)
		{
			LOGE("%s trs_task_thread init failed\n");
			goto error;
		}
	}

	return kNoErr;

error:

	if (trs_task_queue)
	{
		rtos_deinit_queue(&trs_task_queue);
		trs_task_queue = NULL;
	}

	if (trs_task_thread)
	{
		trs_task_queue = NULL;
		rtos_delete_thread(NULL);
	}

	return kGeneralErr;
}

bk_err_t trs_task_send_msg(uint8_t msg_type, uint32_t data)
{
	bk_err_t ret;
	trs_task_msg_t msg;

	if (trs_task_queue)
	{
		msg.type = msg_type;
		msg.data = data;

		ret = rtos_push_to_queue(&trs_task_queue, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret)
		{
			os_printf("video_transfer_cpu1_send_msg failed\r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}


void trs_task_stop(void)
{
	trs_task_send_msg(TRS_TRANSFER_EXIT, 0);
	while (trs_task_thread) {
		rtos_delay_milliseconds(10);
	}
}
