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

#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief miscellaneous APIs
 * @defgroup bk_api_misc miscellaneous group
 * @{
 */

/**
 * @brief  Reboot the system
 *
 * This function reset the system by triggering the interrupt watchdog.
 */
void bk_reboot(void);

/**
 * @brief  Set the base MAC of the system
 *
 * The base MAC address is used to generate the 
 *
 * @attention 1. Generally we should NOT set the MAC and the MAC is read
 *               from the efuse. This API is used for debug only.
 *
 * @return
 *  - BK_OK: success
 *  - BK_ERR_NULL_PARAM: mac is NULL
 *  - BK_ERR_PARAM: mac is invalid
 */
bk_err_t bk_set_mac(const uint8_t *mac);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
