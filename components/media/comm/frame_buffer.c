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

#include "media_core.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>
#include <soc/mapping.h>
#include <driver/media_types.h>

#include "frame_buffer.h"

#define TAG "frame_buffer"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

frame_buffer_t dvp_frame[JPEG_ENC_FRAME_COUNT] = {0};

frame_buffer_info_t *frame_buffer_info = NULL;

volatile uint8_t lcd_frame_lock = false;
volatile uint8_t wifi_frame_lock = false;

volatile uint8_t capture_frame_lock = true;


void dvp_dump(void)
{
	int i;

	for (i = 0; i < JPEG_ENC_FRAME_COUNT; i++)
	{
		os_printf("DUMP, frame[i]: %p id: %d, state: %d, lock: %d\n", i, &dvp_frame[i], dvp_frame[i].id, dvp_frame[i].state, dvp_frame[i].lock);
	}
}

void frame_buffer_free(frame_buffer_t *buffer)
{
	buffer->state = STATE_FRAMED;
}


frame_buffer_t *frame_buffer_alloc(void)
{
	frame_buffer_t *buffer = NULL;
	uint32_t i;

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

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

	GLOBAL_INT_RESTORE();

	if (buffer != NULL)
	{
		buffer->state = STATE_ALLOCED;
	}
	else
	{
		LOGE("%s NULL\n");
		dvp_dump();
	}

	//LOGI("id: %d\n", buffer->id);

	return buffer;
}

frame_buffer_t *frame_buffer_get_available_frame(void)
{
	frame_buffer_t *buffer = NULL;
	uint32_t i;

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

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

	GLOBAL_INT_RESTORE();

	if (buffer != NULL)
	{
		buffer->state = STATE_ALLOCED;
	}

	return buffer;
}


void frame_buffer_complete_notify(frame_buffer_t *buffer)
{
	if (true == frame_buffer_info->wifi_register
	    && (!frame_buffer_info->wifi_lock)
	    && frame_buffer_info->wifi_comp_cb)
	{
		buffer->lock++;
		frame_buffer_info->wifi_lock = true;
		frame_buffer_info->wifi_comp_cb(buffer);
	}

#ifdef CONFIG_LCD

	if (true == frame_buffer_info->lcd_register
	    && (!frame_buffer_info->lcd_lock)
	    && frame_buffer_info->lcd_comp_cb)
	{
		buffer->lock++;
		frame_buffer_info->lcd_lock = true;
		frame_buffer_info->lcd_comp_cb(buffer);
		//LOGD("lcd alloc %p %d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock);
		//lcd_frame_complete_notify(buffer);
	}

#endif

	if (true == frame_buffer_info->capture_register
	    && (!frame_buffer_info->capture_lock)
	    && frame_buffer_info->capture_comp_cb)
	{
		buffer->lock++;
		frame_buffer_info->capture_lock = true;
		frame_buffer_info->capture_comp_cb(buffer);
		//storage_capture_frame_notify(buffer);
	}

	if (!buffer->lock)
	{
		frame_buffer_free(buffer);
	}
}

bool is_workflow_freezing(void)
{
	bool ret = true;

	if (true == frame_buffer_info->wifi_register
	    && (!frame_buffer_info->wifi_lock)
	    && frame_buffer_info->wifi_comp_cb)
	{
		ret = false;
	}

	if (true == frame_buffer_info->lcd_register
	    && (!frame_buffer_info->lcd_lock)
	    && frame_buffer_info->lcd_comp_cb)
	{
		ret = false;
	}

	if (true == frame_buffer_info->capture_register
	    && (!frame_buffer_info->capture_lock)
	    && frame_buffer_info->capture_comp_cb)
	{
		ret = false;
	}

	return ret;
}

void frame_buffer_generate_complete(frame_buffer_t *buffer)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	frame_buffer_free(buffer);

	LOGD("frame[%d]: %u complete\n", buffer->id, buffer->sequence);

	if (!is_workflow_freezing())
	{
		frame_buffer_t *frame = frame_buffer_get_available_frame();
		frame_buffer_complete_notify(frame);
	}

	GLOBAL_INT_RESTORE();
}

void frame_buffer_frame_register(frame_module_t module, void *callback)
{
	switch (module)
	{
		case MODULE_WIFI:
			frame_buffer_info->wifi_register = true;
			frame_buffer_info->wifi_comp_cb = callback;
			break;
		case MODULE_LCD:
			frame_buffer_info->lcd_register = true;
			frame_buffer_info->lcd_comp_cb = callback;
			break;
		case MODULE_RECODER:
			frame_buffer_info->recoder_register = true;
			frame_buffer_info->recoder_comp_cb = callback;
			break;
		case MODULE_CAPTURE:
			frame_buffer_info->capture_register = true;
			frame_buffer_info->capture_lock = false;
			frame_buffer_info->capture_comp_cb = callback;
			break;
	}
}

void frame_buffer_frame_deregister(frame_module_t module)
{
	switch (module)
	{
		case MODULE_WIFI:
			frame_buffer_info->wifi_register = false;
			frame_buffer_info->wifi_lock = false;
			frame_buffer_info->wifi_comp_cb = NULL;
			break;
		case MODULE_LCD:
			frame_buffer_info->lcd_register = false;
			frame_buffer_info->lcd_lock = false;
			frame_buffer_info->lcd_comp_cb = NULL;
			break;
		case MODULE_RECODER:
			frame_buffer_info->recoder_register = false;
			frame_buffer_info->recoder_lock = false;
			frame_buffer_info->recoder_comp_cb = NULL;
			break;
		case MODULE_CAPTURE:
			frame_buffer_info->capture_register = false;
			frame_buffer_info->capture_lock = true;
			frame_buffer_info->capture_comp_cb = NULL;
			break;
	}
}


void frame_buffer_free_request(frame_buffer_t *buffer, frame_module_t module)
{
	GLOBAL_INT_DECLARATION();

	if (buffer->lock == 0)
	{
		LOGE("%s invalid frame free\n", __func__);
		GLOBAL_INT_DISABLE();
		return;
	}


	GLOBAL_INT_DISABLE();

	buffer->lock--;

	if (!buffer->lock)
	{
		frame_buffer_free(buffer);
	}

	switch (module)
	{
		case MODULE_WIFI:
			frame_buffer_info->wifi_lock = false;
			LOGD("wifi free %p %d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock);
			break;
		case MODULE_LCD:
			frame_buffer_info->lcd_lock = false;
			LOGD("lcd free %p %d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock);
			break;
		case MODULE_RECODER:
			break;
		case MODULE_CAPTURE:
			LOGD("capture free %p %d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock);
			break;
	}

	GLOBAL_INT_RESTORE();
}


void frame_buffer_init(void)
{
	int i;

	if (frame_buffer_info == NULL)
	{
		frame_buffer_info = (frame_buffer_info_t*)os_malloc(sizeof(frame_buffer_info_t));
		os_memset((void*)frame_buffer_info, 0, sizeof(frame_buffer_info_t));
	}

	os_memset((void*)dvp_frame, 0, sizeof(dvp_frame));

	for (i = 0; i < JPEG_ENC_FRAME_COUNT; i++)
	{
		dvp_frame[i].state = STATE_INVALID;
		dvp_frame[i].frame = psram_map->jpeg_enc[i];
		dvp_frame[i].size = sizeof(psram_map->jpeg_enc[i]);
		dvp_frame[i].id = i;
	}
}

bool frame_buffer_get_state(void)
{
	bool ret = false;

	if (frame_buffer_info)
	{
		ret = frame_buffer_info->enable;
	}

	return ret;
}

void frame_buffer_enable(bool enable)
{
	int i;

	if (frame_buffer_info)
	{
		os_memset((void*)frame_buffer_info, 0, sizeof(frame_buffer_info_t));

		frame_buffer_info->enable = enable;

		for (i = 0; i < JPEG_ENC_FRAME_COUNT; i++)
		{
			dvp_frame[i].state = STATE_INVALID;
			dvp_frame[i].frame = psram_map->jpeg_enc[i];
			dvp_frame[i].size = sizeof(psram_map->jpeg_enc[i]);
			dvp_frame[i].id = i;
			dvp_frame[i].sequence = 0;
			dvp_frame[i].lock = 0;
		}
	}
}


void frame_buffer_deinit(void)
{
	os_memset((void*)dvp_frame, 0, sizeof(frame_buffer_t) * 3);
}
