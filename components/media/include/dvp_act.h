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

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
	DVP_STATE_DISABLED,
	DVP_STATE_ENABLED,
} dvp_state_t;


typedef struct
{
	dvp_state_t state;
	uint32_t param;
} dvp_info_t;

typedef enum
{
	EVENT_DVP_OPEN_IND = (DVP_EVENT << MEDIA_EVT_BIT),
	EVENT_DVP_CLOSE_IND,
	EVENT_DVP_LCD_REG_CAM_INIT_REQ
} dvp_event_t;

void dvp_camera_event_handle(uint32_t event, uint32_t param);
void set_dvp_camera_state(dvp_state_t state);
dvp_state_t get_dvp_camera_state(void);

void dvp_camera_init(void);

#ifdef __cplusplus
}
#endif
