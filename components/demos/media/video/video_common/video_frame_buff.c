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

#include "video_transfer_cpu1.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

frame_information_t *media_info = NULL;

bk_err_t video_transfer_frame_buff_init(void)
{
	media_info = (frame_information_t *)os_malloc(3 * sizeof(frame_information_t));
	if (media_info == NULL)
		return BK_FAIL;

	os_memset(media_info, 0, 3 * sizeof(frame_information_t));
	media_info[0].buffer_addr = IMAGE_1;
	media_info[0].buffer_id = 0;
	media_info[0].buffer_state = BUFF_IDLE;
	media_info[0].buffer_len = IMAGE_LEN;//200K
	media_info[0].already_len = 0;
	media_info[1].buffer_addr = IMAGE_2;
	media_info[1].buffer_id = 1;
	media_info[1].buffer_state = BUFF_IDLE;
	media_info[1].buffer_len = IMAGE_LEN;//200K
	media_info[1].already_len = 0;
	media_info[2].buffer_addr = IMAGE_3;
	media_info[2].buffer_id = 2;
	media_info[2].buffer_state = BUFF_IDLE;
	media_info[2].buffer_len = IMAGE_LEN;//200K
	media_info[2].already_len = 0;

	return BK_OK;
}

bk_err_t video_transfer_get_idle_buff(uint8_t id, frame_information_t *info)
{
	uint8_t cnt = 0;
	if (info == NULL) {
		os_printf("info memory malloc\r\n");
		return BK_FAIL;
	}

	while (media_info[id].buffer_state != BUFF_IDLE && cnt < 3) {
		cnt++;
		id++;
		if (id == 3)
			id = 0;
	}

	if (cnt == 3) {
		//os_printf("NO idle frame buff\r\n");
		return BK_FAIL;
	}

	os_memcpy(info, &media_info[id], sizeof(frame_information_t));

	return BK_OK;
}

/*
bk_err_t video_transfer_get_ready_buff(uint8_t id, frame_information_t *info)
{
	uint8_t cnt = 0;
	if (info == NULL)
		return BK_FAIL;

	while (media_info[id].buffer_state != BUFF_READY && cnt < 3) {
		cnt++;
		id++;
		if (id == 3)
			id = 0;
	}

	if (cnt == 3) {
		os_printf("NO ready frame buff\r\n");
		return BK_FAIL;
	}

	os_memcpy(info, &media_info[id], sizeof(frame_information_t));

	return BK_OK;
}*/

bk_err_t video_transfer_get_ready_buff(frame_information_t *info)
{
	uint8_t id = 0;
	uint32_t frame_id = 0xFFFFFFFF;

	if (info == NULL)
		return BK_FAIL;

	for (uint8_t cnt = 0; cnt < 3; cnt++) {
		if (media_info[cnt].buffer_state == BUFF_READY) {
			if (frame_id > media_info[cnt].frame_id) {
				id = cnt;
				frame_id = media_info[cnt].frame_id;
			}
		}
	}

	if (frame_id == 0xFFFFFFFF)
		return BK_FAIL;

	os_memcpy(info, &media_info[id], sizeof(frame_information_t));

	return BK_OK;
}



void video_transfer_set_buff_alread_copy_len(uint8_t id, uint32_t copy_len)
{
	media_info[id].already_len += copy_len;
}

void video_transfer_set_buff_state(uint8_t id, frame_buff_state_t state)
{
	media_info[id].buffer_state = state;
	if (state == BUFF_IDLE) {
		media_info[id].already_len = 0;
		//media_info[id].frame_id = 0;
		//media_info[id].frame_len = 0;
	}
}

void video_transfer_set_buff_frame_len(uint8_t id, uint32_t frame_len, uint32_t frame_id)
{
	media_info[id].frame_len = frame_len;
	media_info[id].frame_id = frame_id;
	media_info[id].already_len = 0;
}

void video_transfer_get_buff(uint8_t id, frame_information_t *info)
{
	os_memcpy(info, &media_info[id], sizeof(frame_information_t));
}

void video_transfer_buff_deinit(void)
{
	if (media_info)
		os_free(media_info);

	media_info = NULL;
}

