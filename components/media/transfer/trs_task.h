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

typedef struct
{
	uint8_t type;
	uint32_t data;
} trs_task_msg_t;

typedef enum
{
	TRS_TRANSFER_DATA,
	TRS_TRANSFER_EXIT,
} trs_task_msg_type_t;

typedef struct
{
	uint8_t id;
	uint8_t eof;
	uint8_t cnt;
	uint8_t size;
	uint8_t data[];
} transfer_data_t;

int trs_task_start(video_setup_t *setup_cfg);
void trs_task_stop(void);
bk_err_t trs_task_send_msg(uint8_t msg_type, uint32_t data);


#ifdef __cplusplus
}
#endif
