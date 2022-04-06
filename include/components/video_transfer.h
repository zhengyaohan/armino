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

#include <components/video_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (CONFIG_SPIDMA || CONFIG_CAMERA)

/**
 * @brief     tvideo send msg
 *
 * This API send msg to video thread msg queue, to control video data transfer or stop
 *
 * param new_msg: the type of send msg
 *
 * @return
 *    - void
 */
void tvideo_intfer_send_msg(UINT32 new_msg);

/**
 * @brief     video transfer init
 *
 * This API will create video thread, init msg queue, and excute camera init
 *
 * param setup_cfg: configure of the video transfer include packet process, method of transfer, etc.
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
int video_transfer_init(TVIDEO_SETUP_DESC_PTR setup_cfg);

/**
 * @brief     video transfer deinit
 *
 * This API will quit video thread, and free all reasource
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
int video_transfer_deinit(void);

/**
 * @brief     set video transfer param
 *
 * This API will modify jpeg encode image resolution and camera pps and fps
 *
 * param ppi: image resolution
 * param fps: frame rate
 *
 * @attention 1. only work for gc0328c
 *
 * @return
 *    - 0: succeed
 *    - 1: other errors.
 */
UINT32 video_transfer_set_video_param(UINT32 ppi, UINT32 fps);

/**
 * @brief     set video transfer param
 *
 * This API will open video tranfer data_buffer, and star transfer
 *
 * @attention 1. when call this function, the video date will transfer to wifi(UDP/others)
 *
 * @return
 *    - 0: succeed
 *    - other: other errors.
 */
int video_buffer_open(void);

/**
 * @brief     set video buffer close
 *
 * This API will deinit video tranfer, stop transfer video data
 *
 * @return
 *    - 0: succeed
 *    - other: other errors.
 */
int video_buffer_close(void);

/**
 * @brief     read video buffer frame
 *
 * This API will malloc a data_buffer, and save video data to this buffer
 *
 * param buf: malloc buf pointer
 * param buf_len: buf length
 *
 * @return
 *    - 0: succeed
 *    - other: other errors.
 */
UINT32 video_buffer_read_frame(UINT8 *buf, UINT32 buf_len);
#endif

#ifdef __cplusplus
}
#endif