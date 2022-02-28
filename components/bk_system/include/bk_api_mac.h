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

#include "bk_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MAC_TYPE_BASE = 0,
	MAC_TYPE_STA,
	MAC_TYPE_AP,
	MAC_TYPE_BLE,
	MAC_MAX,
} mac_type_t;

#define BK_ERR_INVALID_MAC_TYPE   (BK_ERR_MAC_BASE)
#define BK_ERR_ZERO_MAC           (BK_ERR_MAC_BASE - 1)
#define BK_ERR_GROUP_MAC          (BK_ERR_MAC_BASE - 2)
#define BK_ERR_INVALID_MAC        (BK_ERR_MAC_BASE - 3)

#define BK_MAC_ADDR_LEN           6

#define BK_IS_ZERO_MAC(m) (((m)[0] == 0) && ((m)[1] == 0) && ((m)[2] == 0) && ((m)[3] == 0)\
	&& ((m)[4] == 0) && ((m)[5] == 0))
#define BK_IS_GROUP_MAC(m) (((m)[0] & 0x01))

/**
 * @brief  Set the base MAC of the system
 *
 * The base MAC address is used to generate the MAC of other interfaces,
 * such as WiFi STA, WiFi AP, BLE, BT etc.
 *
 * @return
 *  - BK_OK: success
 *  - BK_ERR_NULL_PARAM: mac is NULL
 *  - BK_ERR_GROUP_MAC: mac is group address mac
 *  - others: other failures
 */
bk_err_t bk_set_base_mac(const uint8_t *mac);

/**
 * @brief  Get the MAC
 *
 * @return
 *  - BK_OK: success
 *  - BK_ERR_NULL_PARAM: parameter mac is NULL
 *  - BK_ERR_INVALID_MAC_TYPE: mac is not supported
 */
bk_err_t bk_get_mac(uint8_t *mac, mac_type_t type);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
