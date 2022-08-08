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

#ifdef CONFIG_PSRAM
#define FRAME_BUFFER_DISPLAY_COUNT (DISPLAY_FRAME_COUNT)
#define FRAME_BUFFER_JPEG_COUNT (JPEG_ENC_FRAME_COUNT)
frame_buffer_t psram_frame[DISPLAY_FRAME_COUNT + JPEG_ENC_FRAME_COUNT] = {0};
#else
#define FRAME_BUFFER_DISPLAY_COUNT (0)
#define FRAME_BUFFER_JPEG_COUNT (1)
frame_buffer_t psram_frame[FRAME_BUFFER_JPEG_COUNT] = {0};
#endif

frame_buffer_info_t *frame_buffer_info = NULL;



void dvp_dump(void)
{
	int i;

	for (i = 0; i < FRAME_BUFFER_DISPLAY_COUNT + FRAME_BUFFER_JPEG_COUNT; i++)
	{
		os_printf("DUMP, frame[%d]: %p id: %d, state: %d, lock: %d\n", i, &psram_frame[i], psram_frame[i].id, psram_frame[i].state, psram_frame[i].lock);
	}
}

void frame_buffer_free(frame_buffer_t *buffer)
{
	buffer->state = STATE_FRAMED;
}


frame_buffer_t *frame_buffer_alloc(frame_type_t type)
{
	frame_buffer_t *buffer = NULL;
	uint32_t i, start = 0, end = 0;

	if (type == FRAME_DISPLAY)
	{
		start = 0;
		end = FRAME_BUFFER_DISPLAY_COUNT;
	}
	else if (type == FRAME_JPEG)
	{
		start = FRAME_BUFFER_DISPLAY_COUNT;
		end = FRAME_BUFFER_DISPLAY_COUNT + FRAME_BUFFER_JPEG_COUNT;
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

	for (i = start; i < end; i++)
	{
		if (psram_frame[i].state == STATE_ALLOCED)
		{
			continue;
		}

		if (buffer == NULL)
		{
			buffer = &psram_frame[i];
			continue;
		}

		if (buffer->sequence > psram_frame[i].sequence)
		{
			buffer = &psram_frame[i];
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

frame_buffer_t *frame_buffer_get_available_frame(frame_type_t type)
{
	frame_buffer_t *buffer = NULL;
	uint32_t i, start = 0, end = 0;

	if (type == FRAME_DISPLAY)
	{
		start = 0;
		end = FRAME_BUFFER_DISPLAY_COUNT;
	}
	else if (type == FRAME_JPEG)
	{
		start = FRAME_BUFFER_DISPLAY_COUNT;
		end = FRAME_BUFFER_DISPLAY_COUNT + FRAME_BUFFER_JPEG_COUNT;
	}

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

	for (i = start; i < end; i++)
	{
		if (psram_frame[i].state != STATE_FRAMED)
		{
			continue;
		}

		if (buffer == NULL)
		{
			buffer = &psram_frame[i];
			continue;
		}

		if (buffer->sequence < psram_frame[i].sequence)
		{
			buffer = &psram_frame[i];
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
	if (buffer->type == FRAME_JPEG)
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

		if (true == frame_buffer_info->decoder_register
		    && (!frame_buffer_info->decoder_lock)
		    && frame_buffer_info->decoder_comp_cb)
		{
			buffer->lock++;
			frame_buffer_info->decoder_lock = true;
			frame_buffer_info->decoder_comp_cb(buffer);
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
	}

	if (buffer->type == FRAME_DISPLAY)
	{
#ifdef CONFIG_LCD

		if (true == frame_buffer_info->display_register
		    && (!frame_buffer_info->display_lock)
		    && frame_buffer_info->display_comp_cb)
		{
			buffer->lock++;
			//frame_buffer_info->display_lock = true;
			frame_buffer_info->display_comp_cb(buffer);
		}

#endif
	}

	if (!buffer->lock)
	{
		frame_buffer_free(buffer);
	}
}

bool is_workflow_freezing(frame_type_t type)
{
	bool ret = true;

	LOGD("WIFI %d:%d, DEC %d:%d\n",
	     frame_buffer_info->wifi_register, frame_buffer_info->wifi_lock,
	     frame_buffer_info->decoder_register, frame_buffer_info->decoder_lock);

	if (type == FRAME_JPEG)
	{
		if (true == frame_buffer_info->wifi_register
		    && (!frame_buffer_info->wifi_lock)
		    && frame_buffer_info->wifi_comp_cb)
		{
			ret = false;
		}

		if (true == frame_buffer_info->decoder_register
		    && (!frame_buffer_info->decoder_lock)
		    && frame_buffer_info->decoder_comp_cb)
		{
			ret = false;
		}

		if (true == frame_buffer_info->capture_register
		    && (!frame_buffer_info->capture_lock)
		    && frame_buffer_info->capture_comp_cb)
		{
			ret = false;
		}
	}

	if (type == FRAME_DISPLAY)
	{
		if (true == frame_buffer_info->display_register
		    && (!frame_buffer_info->display_lock)
		    && frame_buffer_info->display_comp_cb)
		{
			ret = false;
		}
	}

	return ret;
}

void frame_buffer_generate_complete(frame_buffer_t *buffer, frame_type_t type)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	frame_buffer_free(buffer);

	if (!is_workflow_freezing(type))
	{
		LOGD("notify frame[%d]: %u complete, %d\n", buffer->id, buffer->sequence, type);

		frame_buffer_t *frame = frame_buffer_get_available_frame(type);
		frame_buffer_complete_notify(frame);
	}
	else
	{
		LOGD("covery frame[%d]: %u complete, %d\n", buffer->id, buffer->sequence, type);
	}

	GLOBAL_INT_RESTORE();
}

void frame_buffer_frame_register(frame_module_t module, void *callback)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	switch (module)
	{
		case MODULE_WIFI:
			frame_buffer_info->wifi_register = true;
			frame_buffer_info->wifi_comp_cb = callback;
			break;
		case MODULE_DECODER:
			frame_buffer_info->decoder_register = true;
			frame_buffer_info->decoder_comp_cb = callback;
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
		case MODULE_DISPLAY:
			frame_buffer_info->display_register = true;
			frame_buffer_info->display_comp_cb = callback;
			break;
	}

	GLOBAL_INT_RESTORE();
}

void frame_buffer_frame_deregister(frame_module_t module)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	switch (module)
	{
		case MODULE_WIFI:
			frame_buffer_info->wifi_register = false;
			frame_buffer_info->wifi_lock = false;
			frame_buffer_info->wifi_comp_cb = NULL;
			break;
		case MODULE_DECODER:
			frame_buffer_info->decoder_register = false;
			frame_buffer_info->decoder_lock = false;
			frame_buffer_info->decoder_comp_cb = NULL;
			break;
		case MODULE_RECODER:
			frame_buffer_info->recoder_register = false;
			frame_buffer_info->recoder_lock = false;
			frame_buffer_info->recoder_comp_cb = NULL;
			break;
		case MODULE_CAPTURE:
			frame_buffer_info->capture_register = false;
			frame_buffer_info->capture_lock = false;
			frame_buffer_info->capture_comp_cb = NULL;
			break;
		case MODULE_DISPLAY:
			frame_buffer_info->capture_register = false;
			frame_buffer_info->capture_lock = false;
			frame_buffer_info->capture_comp_cb = NULL;
			break;
	}

	GLOBAL_INT_RESTORE();
}


void frame_buffer_free_request(frame_buffer_t *buffer, frame_module_t module)
{
	GLOBAL_INT_DECLARATION();

	if (buffer->lock == 0)
	{
		LOGE("%s invalid frame free\n", __func__);
		return;
	}


	GLOBAL_INT_DISABLE();

	buffer->lock--;

	if (!buffer->lock)
	{
		LOGD("free frame[%d] %d\n", buffer->id, buffer->sequence);
		frame_buffer_free(buffer);
	}

	switch (module)
	{
		case MODULE_WIFI:
			frame_buffer_info->wifi_lock = false;
			LOGD("wifi free %p %d:%d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock, buffer->sequence);
			break;
		case MODULE_DECODER:
			frame_buffer_info->decoder_lock = false;
			LOGD("lcd free %p %d:%d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock, buffer->sequence);
			break;
		case MODULE_RECODER:
			break;
		case MODULE_CAPTURE:
			LOGD("capture free %p %d:%d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock, buffer->sequence);
			break;
		case MODULE_DISPLAY:
			frame_buffer_info->display_lock = false;
			LOGD("lcd free %p %d:%d:%d:%d\n", buffer, buffer->id, buffer->state, buffer->lock, buffer->sequence);
			break;
	}

	GLOBAL_INT_RESTORE();
}


void frame_buffer_init(void)
{
	if (frame_buffer_info == NULL)
	{
		frame_buffer_info = (frame_buffer_info_t *)os_malloc(sizeof(frame_buffer_info_t));
		os_memset((void *)frame_buffer_info, 0, sizeof(frame_buffer_info_t));
	}

	os_memset((void *)psram_frame, 0, sizeof(psram_frame));

#if (CONFIG_PSRAM)
	int i;

	for (i = 0; i < FRAME_BUFFER_DISPLAY_COUNT; i++)
	{
		psram_frame[i].state = STATE_INVALID;
		psram_frame[i].frame = psram_map->display[i];
		psram_frame[i].size = sizeof(psram_map->display[i]);
		psram_frame[i].id = i;
		psram_frame[i].type = FRAME_DISPLAY;
	}

	for (i = FRAME_BUFFER_DISPLAY_COUNT; i < FRAME_BUFFER_DISPLAY_COUNT + FRAME_BUFFER_JPEG_COUNT; i++)
	{
		psram_frame[i].state = STATE_INVALID;
		psram_frame[i].frame = psram_map->jpeg_enc[i];
		psram_frame[i].size = sizeof(psram_map->jpeg_enc[i]);
		psram_frame[i].id = i;
		psram_frame[i].type = FRAME_JPEG;
	}

#else

#endif

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
	if (frame_buffer_info)
	{
		os_memset((void *)frame_buffer_info, 0, sizeof(frame_buffer_info_t));

		frame_buffer_info->enable = enable;

#if (CONFIG_PSRAM)
		int i;

		for (i = 0; i < FRAME_BUFFER_DISPLAY_COUNT; i++)
		{
			psram_frame[i].state = STATE_INVALID;
			psram_frame[i].frame = psram_map->display[i];
			psram_frame[i].size = sizeof(psram_map->display[i]);
			psram_frame[i].id = i;
			psram_frame[i].type = FRAME_DISPLAY;
			psram_frame[i].length = 0;
			psram_frame[i].sequence = 0;
			psram_frame[i].lock = 0;

			LOGI("display frame[%d]: ptr: %p, size: %u\n", i, psram_frame[i].frame, psram_frame[i].size);
		}

		for (i = FRAME_BUFFER_DISPLAY_COUNT; i < FRAME_BUFFER_DISPLAY_COUNT + FRAME_BUFFER_JPEG_COUNT; i++)
		{
			psram_frame[i].state = STATE_INVALID;
			psram_frame[i].frame = psram_map->jpeg_enc[i];
			psram_frame[i].size = sizeof(psram_map->jpeg_enc[i]);
			psram_frame[i].id = i;
			psram_frame[i].type = FRAME_JPEG;
			psram_frame[i].length = 0;
			psram_frame[i].sequence = 0;
			psram_frame[i].lock = 0;

			LOGI("jpeg frame[%d]: ptr: %p, size: %u\n", i, psram_frame[i].frame, psram_frame[i].size);
		}

#endif

	}
}

void frame_buffer_display_reset(void)
{
#if (CONFIG_PSRAM)
	int i;

	for (i = 0; i < FRAME_BUFFER_DISPLAY_COUNT; i++)
	{
		psram_frame[i].state = STATE_INVALID;
		psram_frame[i].frame = psram_map->display[i];
		psram_frame[i].size = sizeof(psram_map->display[i]);
		psram_frame[i].id = i;
		psram_frame[i].type = FRAME_DISPLAY;
		psram_frame[i].length = 0;
		psram_frame[i].sequence = 0;
		psram_frame[i].lock = 0;

		LOGI("display frame[%d]: ptr: %p, size: %u\n", i, psram_frame[i].frame, psram_frame[i].size);
	}
#endif
}


void frame_buffer_deinit(void)
{
	os_memset((void *)psram_frame, 0, sizeof(frame_buffer_t) * 3);

	if (frame_buffer_info)
	{
		os_free(frame_buffer_info);
		frame_buffer_info = NULL;
	}
}
