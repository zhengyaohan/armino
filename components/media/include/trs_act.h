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

#include <common/bk_include.h>
#include <driver/media_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	TRS_STATE_DISABLED,
	TRS_STATE_ENABLED,
} trs_state_t;


typedef struct
{
	trs_state_t state;
	uint32_t param;
} trs_info_t;


typedef enum
{
	EVENT_TRS_VIDEO_TRANSFER_OPEN_IND = (TRS_EVENT << MEDIA_EVT_BIT),
	EVENT_TRS_VIDEO_TRANSFER_CLOSE_IND,
	EVENT_TRS_FRAME_COMPLETE_IND,
	EVENT_TRS_FRAME_FREE_IND,
	EVENT_TRS_FRAME_REGISTER_IND
} transfer_event_t;

typedef enum
{
	MODULE_LCD,
} transfer_module_t;


void wifi_transfer_event_handle(uint32_t event, uint32_t param);
void set_trs_video_transfer_state(trs_state_t state);
trs_state_t get_trs_video_transfer_state(void);
void trs_video_transfer_init(void);

frame_buffer_t *get_trs_transfer_lock_frame(void);
void set_trs_transfer_lock_frame(frame_buffer_t *frame);
void video_transfer_frame_register(transfer_module_t module);


#ifdef __cplusplus
}
#endif
