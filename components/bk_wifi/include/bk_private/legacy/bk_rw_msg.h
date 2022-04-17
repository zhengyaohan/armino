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

//TODO we should finally remove RW_EVT_ and use EVENT_ instead!!!

typedef enum {
	/* for station mode */
	RW_EVT_STA_IDLE = 0,
	RW_EVT_STA_CONNECTING,
	RW_EVT_STA_BEACON_LOSE,
	RW_EVT_STA_PASSWORD_WRONG,
	RW_EVT_STA_NO_AP_FOUND,
	RW_EVT_STA_DHCP_TIMEOUT,
	RW_EVT_STA_ASSOC_FULL,
	RW_EVT_STA_DISCONNECTED,    /* disconnect with server */
	RW_EVT_STA_CONNECT_FAILED, /* authentication failed */
	RW_EVT_STA_CONNECTED,	 /* authentication success */
	RW_EVT_STA_GOT_IP,
	/* for softap mode */
	RW_EVT_AP_CONNECTED,          /* a client association success */
	RW_EVT_AP_DISCONNECTED,    /* a client disconnect */
	RW_EVT_AP_CONNECT_FAILED, /* a client association failed */
	RW_EVT_MAX
} rw_evt_type;

void mhdr_set_station_status(rw_evt_type val);
rw_evt_type mhdr_get_station_status(void);
void rwm_mgmt_set_vif_netif(uint8_t *mac, void *netif);

#ifdef __cplusplus
}
#endif
