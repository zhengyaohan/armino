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

#include <stdio.h>
#include <string.h>

#include <os/os.h>
#include "mailbox_channel.h"
#include "mb_ipc_cmd.h"
#include "amp_res_lock.h"
#include "amp_lock_api.h"

typedef struct
{
	u8					inited;
	beken_semaphore_t	res_sema;

#if CONFIG_MASTER_CORE
	amp_res_req_cnt_t	res_cnt_list;
#endif

} amp_res_sync_t;

static amp_res_sync_t	amp_res_sync[AMP_RES_ID_MAX];

#if CONFIG_MASTER_CORE

/* call this API in interrupt disabled state. */
bk_err_t amp_res_acquire_cnt(u16 res_id, u16 cpu_id, amp_res_req_cnt_t *cnt_list)
{
	if(res_id >= AMP_RES_ID_MAX)
		return BK_ERR_PARAM;

	if(amp_res_sync[res_id].inited == 0)
		return BK_ERR_NOT_INIT;

	if(cpu_id >= AMP_CPU_CNT)
		return BK_ERR_PARAM;

	u16		i = 0;

	for(i = 0; i < AMP_CPU_CNT; i++)
	{
		cnt_list->req_cnt[i] = amp_res_sync[res_id].res_cnt_list.req_cnt[i];
	}

	amp_res_sync[res_id].res_cnt_list.req_cnt[cpu_id]++;

	return BK_OK;
	
}

/* call this API in interrupt disabled state. */
bk_err_t amp_res_release_cnt(u16 res_id, u16 cpu_id, amp_res_req_cnt_t *cnt_list)
{
	if(res_id >= AMP_RES_ID_MAX)
		return BK_ERR_PARAM;

	if(amp_res_sync[res_id].inited == 0)
		return BK_ERR_NOT_INIT;

	if(cpu_id >= AMP_CPU_CNT)
		return BK_ERR_PARAM;

	u16		i = 0;

	if(amp_res_sync[res_id].res_cnt_list.req_cnt[cpu_id] > 0)
	{
		amp_res_sync[res_id].res_cnt_list.req_cnt[cpu_id]--;

		for(i = 0; i < AMP_CPU_CNT; i++)
		{
			cnt_list->req_cnt[i] = amp_res_sync[res_id].res_cnt_list.req_cnt[i];
		}

		return BK_OK;
	}
	else
	{
		return BK_FAIL;
	}
	
}

#endif

/* Apps can't call this API, it's for IPC isr only. */
bk_err_t amp_res_available(u16 res_id)
{
	if(res_id >= AMP_RES_ID_MAX)
		return BK_ERR_PARAM;

	if(amp_res_sync[res_id].inited == 0)
		return BK_ERR_NOT_INIT;

	return rtos_set_semaphore(&amp_res_sync[res_id].res_sema);
}

bk_err_t amp_res_acquire(u16 res_id, u32 timeout_ms)
{
	u16 		self_cnt, other_cnt, i;
	bk_err_t	ret_val = BK_FAIL;
	amp_res_req_cnt_t	cnt_list;

	if(res_id >= AMP_RES_ID_MAX)
		return BK_ERR_PARAM;

	if(amp_res_sync[res_id].inited == 0)
		return BK_ERR_NOT_INIT;

#if CONFIG_MASTER_CORE

	u32  int_mask = rtos_disable_int();

	ret_val = amp_res_acquire_cnt(res_id, SRC_CPU, &cnt_list);

	rtos_enable_int(int_mask);

#endif

#if CONFIG_SLAVE_CORE

	ret_val = ipc_send_res_acquire_cnt(res_id, SRC_CPU, &cnt_list);

#endif

	if(ret_val != BK_OK)
	{
		return ret_val;
	}

	self_cnt = cnt_list.req_cnt[SRC_CPU];
	other_cnt = 0;

	for(i = 0; i < AMP_CPU_CNT; i++)
	{
		other_cnt += cnt_list.req_cnt[i];
	}
	other_cnt -= self_cnt;

	if((self_cnt == 0) && (other_cnt > 0))
	{
		/* resource was occupied by other CPU, so set semaphore state to unavailable. */
		ret_val = rtos_get_semaphore(&amp_res_sync[res_id].res_sema, 0);

		if(ret_val != BK_OK)
		{
			return ret_val;
		}
	}

	ret_val = rtos_get_semaphore(&amp_res_sync[res_id].res_sema, timeout_ms);

	return ret_val;

}

bk_err_t amp_res_release(u16 res_id)
{
	u16 		self_cnt, other_cnt, i;
	bk_err_t	ret_val = BK_FAIL;
	amp_res_req_cnt_t	cnt_list;

	if(res_id >= AMP_RES_ID_MAX)
		return BK_ERR_PARAM;

	if(amp_res_sync[res_id].inited == 0)
		return BK_ERR_NOT_INIT;

#if CONFIG_MASTER_CORE

	u32  int_mask = rtos_disable_int();

	ret_val = amp_res_release_cnt(res_id, SRC_CPU, &cnt_list);

	rtos_enable_int(int_mask);

#endif

#if CONFIG_SLAVE_CORE

	ret_val = ipc_send_res_release_cnt(res_id, SRC_CPU, &cnt_list);

#endif

	if(ret_val != BK_OK)
	{
		return ret_val;
	}

	self_cnt = cnt_list.req_cnt[SRC_CPU];
	other_cnt = 0;

	for(i = 0; i < AMP_CPU_CNT; i++)
	{
		other_cnt += cnt_list.req_cnt[i];
	}
	other_cnt -= self_cnt;

	if((self_cnt == 0) && (other_cnt > 0))
	{
		/* other CPU is waiting for the resource, so inform CPU that it is available. */
		/* which CPU is selected in multi-cores? (over than 2 cores)*/
		ret_val = ipc_send_available_ind(res_id);

		if(ret_val != BK_OK)
		{
			return ret_val;
		}
	}

	ret_val = rtos_set_semaphore(&amp_res_sync[res_id].res_sema);

	return ret_val;

}

bk_err_t amp_res_init(u16 res_id)
{
	bk_err_t	ret_val = BK_FAIL;

	if(res_id >= AMP_RES_ID_MAX)
		return BK_ERR_PARAM;

	if(amp_res_sync[res_id].inited != 0)
		return BK_OK;

#if CONFIG_MASTER_CORE

	u16 i = 0;

	for(i = 0; i < AMP_CPU_CNT; i++)
	{
		amp_res_sync[res_id].res_cnt_list.req_cnt[i] = 0;
	}

#endif

	ret_val = rtos_init_semaphore_adv(&amp_res_sync[res_id].res_sema, 1, 1);

	if(ret_val != BK_OK)
		return ret_val;

	amp_res_sync[res_id].inited = 1;

	return BK_OK;
}

