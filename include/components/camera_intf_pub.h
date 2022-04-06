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

#ifdef __cplusplus
extern "C" {
#endif

void camera_flip(uint8_t n);

/**
 * @brief     Init the camera
 *
 * This API init jpeg and i2c, enable camera at last
.*
 * param data: configure for camera and jpeg
 *
 * @attention 1. work for imaging transfer
 *
 * @return
 *    - void
 */
void camera_intfer_init(void *data);

/**
 * @brief     deinit the camera
 *
 * This API will close camera function
 *
 * @attention 1. work for imaging transfer
 *
 * @return
 *    - void
 */
void camera_intfer_deinit(void);

/**
 * @brief     set camera config
 *
 * This API will use i2c write camera_init_table, enable camera
 *
 * @return
 *    - void
 */
void camera_intf_config_senser(void);

/**
 * @brief     set video param
 *
 * This API will set jpeg pps and camera pps and fps
 *
 * param ppi_type: image resolution
 * param pfs_type: frame rate
 *
 * @attention 1. only work for gc0328c
 *
 * @return
 *    - void
 */
uint32_t camera_intfer_set_video_param(uint32_t ppi_type, uint32_t pfs_type);


/**
 * @brief     set camera pps and fps
 *
 * This API will set jpeg pps and camera pps and fps
 *
 * param ppi_type: image resolution
 * param pfs_type: frame rate
 *
 * @attention 1. need call before camera_intf_config_senser
 *
 * @return
 *    - void
 */
int camera_set_ppi_fps(uint32_t ppi_type, uint32_t fps_type);
#ifdef __cplusplus
}
#endif
