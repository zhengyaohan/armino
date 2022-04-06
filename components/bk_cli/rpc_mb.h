// Copyright 2020-2022 Beken
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

#ifndef _rpc_h_
#define _rpc_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_typedef.h>

#define FIELD_OFFSET(type, member)		((u32)(&(((type *)0)->member)))
#define FIELD_SIZE(type, member)		(sizeof(((type *)0)->member))

/* FIELD_IDX works only when every member size of type is the SAME ! */
#define FIELD_IDX(type, member)			(FIELD_OFFSET(type, member) / FIELD_SIZE(type, member))


#define RPC_CTRL_NO_RETURN		0x01

typedef union
{
	struct
	{
		u8	mod_id;
		u8	api_id;
		u8	ctrl;
		u8	data_len;
	};
	u32		call_id;
} rpc_call_hdr_t;

typedef struct
{
	rpc_call_hdr_t	call_hdr;
	u8	call_param[0];
} rpc_call_def_t;

typedef struct
{
	rpc_call_hdr_t	call_hdr;
	u8	ret_data[0];		/* api_ret_data_t if has ret data. */
} rpc_ret_def_t;

typedef struct
{
	bk_err_t	ret_val;
	u8			result[0];
} api_ret_data_t;

enum
{
	SIMPLE_TEST_CMD = 0,

	CPU1_POWER_UP,
	GET_POWER_SAVE_FLAG,
	CPU1_HEART_BEAT,
	SET_CPU1_HEART_RATE,

	SET_SPINLOCK_GPIO,
	GET_SPINLOCK_GPIO,
	
	SET_SPINLOCK_DMA,
	GET_SPINLOCK_DMA,
	
};

enum
{
	RPC_MOD_GPIO = 0,
	RPC_MOD_DMA,
	RPC_MOD_MAX,
} ;

bk_err_t rpc_client_init(void);
bk_err_t rpc_client_call(rpc_call_def_t *rpc_param, u16 param_len, api_ret_data_t * ret_buf, u8 buf_len);
bk_err_t client_send_simple_cmd(u32 sub_cmd, u32 param1, u32 param2);

bk_err_t rpc_server_init(void);
bk_err_t rpc_server_rsp(rpc_ret_def_t *rsp_param, u16 param_len);
//int rpc_server_listen_cmd(u32 timeout_ms);
//void rpc_server_handle_cmd(void);
bk_err_t server_send_simple_cmd(u32 sub_cmd, u32 param1, u32 param2);

#ifdef __cplusplus
}
#endif

#endif /* _rpc_h_ */

