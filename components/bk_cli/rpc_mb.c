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
#include "rpc_mb.h"
#include "spinlock.h"


#define MOD_TAG		"RPC"

#define RPC_CALL_TIMEOUT		10		/* 10ms */
#define RPC_XCHG_DATA_MAX		64

typedef union
{
	struct
	{
		mb_chnl_hdr_t	chnl_hdr;
		void *	cmd_param;
		u16		cmd_param_len;
	};
	mb_chnl_cmd_t	cmd_buf;
} rpc_cmd_t;

/*typedef union
{
	struct
	{
		mb_chnl_hdr_t	chnl_hdr;
		void *	rsp_data;
		u16		rsp_data_len;
	};
	mb_chnl_cmd_t	cmd_buf;
} rpc_rsp_t;*/

typedef struct
{
	bk_err_t	ret_val;
	u8			result[RPC_XCHG_DATA_MAX - sizeof(bk_err_t) - sizeof(rpc_call_hdr_t)];
} api_max_ret_t;

typedef struct
{
	rpc_call_hdr_t	call_hdr;
	api_max_ret_t	max_ret_data;
} rpc_max_ret_t;

typedef struct
{
	rpc_call_hdr_t	call_hdr;
	bk_err_t		ret_val;		/* return fail code. */ /* struct of api_ret_data_t */
} rpc_ret_fail_t;

enum
{
	RPC_CALL_CMD = 0,
	RPC_RET_CMD,
	SYS_CTRL_SIMPLE_CMD,
};

typedef struct
{
	u8		chnl_1_param[RPC_XCHG_DATA_MAX];
	u8		chnl_1_ret[RPC_XCHG_DATA_MAX];
} rpc_xchg_buf_t;

//extern char _swap_start;
static rpc_xchg_buf_t * const xchg_buf = (rpc_xchg_buf_t *)(0x3007e000); //(&_swap_start);

#if CONFIG_SLAVE_CORE

/**    ============================      RPC client    ============================   **/

typedef struct
{
	/* chnl data */
	u8					chnl_inited;
	u8					chnl_id;
	beken_semaphore_t	chnl_sema;

	/* client data */
	beken_semaphore_t	api_rsp_sema;
	rpc_call_hdr_t		cur_call_hdr;
	volatile u8		rpc_in_process;
	volatile u8		rpc_call_failed;
	u8				ret_buf_len;
	u8				ret_buf[sizeof(api_max_ret_t)];		/* api_ret_data_t * */
} rpc_client_cb_t;

static rpc_client_cb_t	rpc_client_cb; // = { .chnl_id = MB_CHNL_HW_CTRL, .chnl_inited = 0 };

static bk_err_t client_handle_simple_cmd(mb_chnl_cmd_t * cmd_buf)
{
	/* ack_buf is the cmd_buf.       */
	mb_chnl_ack_t * ack_buf = (mb_chnl_ack_t *)cmd_buf;

	/* must NOT change ack_buf->hdr. */

	BK_LOGI(MOD_TAG, "rpc cmd: %d, %d\r\n", cmd_buf->param2, cmd_buf->param3);

	switch(cmd_buf->param1)  /* sub cmd. */
	{
		case SIMPLE_TEST_CMD:
			ack_buf->ack_data2 += 1;
			ack_buf->ack_data3 += 1;
			break;

		case SET_CPU1_HEART_RATE:
			// save the param.
			break;

		case GET_POWER_SAVE_FLAG:
			// return PS flag to server.
			//ack_buf->ack_data2 = (u32)get_cpu1_ps_flag();
			break;

		case SET_SPINLOCK_GPIO:		// cmd from server to announce spinlock addr.
			// gpio_spinlock_ptr = (spinlock_t *)(cmd_buf->param2);
			//    or
			// set_gpio_spinlock((spinlock_t *)(cmd_buf->param2));  // client save spinlock addr.
			break;

		#if 0
		case GET_SPINLOCK_GPIO: 	// cmd from server, get spinklock addr, return the addr in the first field following sub-cmd.
			ack_buf->ack_data2 = (u32)(&gpio_spinlock);
			break;
		#endif

		default:
			ack_buf->ack_data2 = (u32)(-1);
			ack_buf->ack_data3 = (u32)(-1);
			return BK_FAIL;
			break;
	}

	return BK_OK;
}

static void client_handle_simple_rsp(mb_chnl_ack_t * ack_buf)
{
	BK_LOGI(MOD_TAG, "rpc rsp: %d, %d\r\n", ack_buf->ack_data2, ack_buf->ack_data3);

	switch(ack_buf->ack_data1)  /* sub cmd. */
	{
		case SIMPLE_TEST_CMD:
			break;

		case GET_SPINLOCK_GPIO: // client try to get spinlock addr in server, server return the address, so client save the data.
			{
				extern spinlock_t  *	gpio_spinlock_ptr;
				gpio_spinlock_ptr = (spinlock_t *)ack_buf->ack_data2;
			}
			break;

		case GET_SPINLOCK_DMA:
			break;

		default:
			break;
	}
}

static void rpc_client_rx_isr(rpc_client_cb_t * client_cb, mb_chnl_cmd_t *cmd_buf)
{
	u32		result = ACK_STATE_FAIL;
	mb_chnl_ack_t * ack_buf;

	if(cmd_buf->hdr.cmd == RPC_RET_CMD)  /* call rsp from rpc sever. */
	{
		if(client_cb->rpc_in_process == 0)	/* unsolicited rpc response. */
		{
			goto rpc_client_rx_isr_exit;
		}
		BK_LOGI(MOD_TAG, "RPC_RET_CMD: %d, %d, %d\r\n", cmd_buf->param1, cmd_buf->param2, cmd_buf->param3);

		rpc_cmd_t		* rpc_cmd = (rpc_cmd_t *)cmd_buf;
		rpc_ret_def_t	* rsp_buf = (rpc_ret_def_t *)rpc_cmd->cmd_param;

		if( (client_cb->cur_call_hdr.mod_id != rsp_buf->call_hdr.mod_id) || 
			(client_cb->cur_call_hdr.api_id != rsp_buf->call_hdr.api_id) )
		{
			/* un-matched rpc response. */
			goto rpc_client_rx_isr_exit;
		}

		result = ACK_STATE_COMPLETE;

		client_cb->rpc_in_process  = 0;

		if(client_cb->cur_call_hdr.ctrl & RPC_CTRL_NO_RETURN)
		{
			rtos_set_semaphore(&client_cb->chnl_sema);
		}
		else 
		{
			client_cb->ret_buf_len = rsp_buf->call_hdr.data_len;
			if(client_cb->ret_buf_len > sizeof(api_max_ret_t))
				client_cb->ret_buf_len = sizeof(api_max_ret_t);

			/* return ACK_STATE_COMPLETE, so must copy data from buff in case of the buffer released. */
			memcpy(&client_cb->ret_buf[0], &rsp_buf->ret_data[0], client_cb->ret_buf_len);

			rtos_set_semaphore(&client_cb->api_rsp_sema);
		}

		goto rpc_client_rx_isr_exit;
	}

	if(cmd_buf->hdr.cmd == SYS_CTRL_SIMPLE_CMD)  /* simple cmd for IPC. */
	{
		client_handle_simple_cmd(cmd_buf);

		return;		/* don't change the ack_state of mb_chnl_ack_t, it is used as ack_data3. */
	}

	/*
	 *   !!!  FAULT  !!!
	 */
	BK_LOGE(MOD_TAG, "Fault in %s,cmd:%d\r\n", __func__, cmd_buf->hdr.cmd);

rpc_client_rx_isr_exit:

	/* overwrite the cmd_buf->param3 after the ISR handle complete.
	 * return the ack info to caller using the SAME buffer with cmd buffer.
	 *     !!!! [input as param / output as result ]  !!!!
	 */
	ack_buf = (mb_chnl_ack_t *)cmd_buf;
	ack_buf->ack_state = result;

	return;

}

static void rpc_client_tx_cmpl_isr(rpc_client_cb_t * client_cb, mb_chnl_ack_t *ack_buf)  /* tx_cmpl_isr */
{
	if(ack_buf->hdr.cmd == RPC_CALL_CMD)
	{
		if(client_cb->rpc_in_process == 0)	/* RPC_RET_CMD rx_isr may arrive before this tx_cmpl_isr. */
			return;

		/* RPC cmd tx complete. */

		if( (ack_buf->hdr.state & CHNL_STATE_COM_FAIL) 
			|| (ack_buf->ack_state != ACK_STATE_PENDING) )
		{
			client_cb->rpc_call_failed = 1;
			client_cb->rpc_in_process  = 0;

			/* communication failed or function failed or func completed. */
			if(client_cb->cur_call_hdr.ctrl & RPC_CTRL_NO_RETURN)
			{
				rtos_set_semaphore(&client_cb->chnl_sema);
			}
			else
			{
				rtos_set_semaphore(&client_cb->api_rsp_sema);
			}
		}
		else
		{
			/* communication ok, function is in handling & will send RPC_RET_CMD in later. */
		}

		return;
	}

	if(ack_buf->hdr.cmd == SYS_CTRL_SIMPLE_CMD)  /* simple cmd for IPC. */
	{
		client_handle_simple_rsp(ack_buf);
		rtos_set_semaphore(&client_cb->chnl_sema);
		return;
	}

	/*
	 *   !!!  FAULT  !!!
	 */
	BK_LOGE(MOD_TAG, "Fault in %s,cmd:%d\r\n", __func__, ack_buf->hdr.cmd);

	return;

}

bk_err_t rpc_client_init(void)
{
	bk_err_t		ret_code;

	u8 chnl_id = MB_CHNL_HW_CTRL;

	if(rpc_client_cb.chnl_inited)
		return BK_OK;

	memset(&rpc_client_cb, 0, sizeof(rpc_client_cb));
	rpc_client_cb.chnl_id = chnl_id;

	ret_code = rtos_init_semaphore_adv(&rpc_client_cb.chnl_sema, 1, 1);
	if(ret_code != BK_OK)
	{
		return ret_code;
	}

	ret_code = rtos_init_semaphore(&rpc_client_cb.api_rsp_sema, 1);
	if(ret_code != BK_OK)
	{
		rtos_deinit_semaphore(&rpc_client_cb.chnl_sema);

		return ret_code;
	}

	ret_code = mb_chnl_open(chnl_id, &rpc_client_cb);
	if(ret_code != BK_OK)
	{
		rtos_deinit_semaphore(&rpc_client_cb.chnl_sema);
		rtos_deinit_semaphore(&rpc_client_cb.api_rsp_sema);

		return ret_code;
	}

	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_RX_ISR, (void *)rpc_client_rx_isr);
	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_CMPL_ISR, (void *)rpc_client_tx_cmpl_isr);

	rpc_client_cb.chnl_inited = 1;

	return BK_OK;
}

bk_err_t rpc_client_call(rpc_call_def_t *rpc_param, u16 param_len, api_ret_data_t * ret_buf, u8 buf_len)
{
	if(!rpc_client_cb.chnl_inited)
		return BK_FAIL;

	rtos_get_semaphore(&rpc_client_cb.chnl_sema, BEKEN_WAIT_FOREVER);

	rpc_client_cb.rpc_in_process  = 1;
	rpc_client_cb.rpc_call_failed = 0;
	rpc_client_cb.cur_call_hdr.call_id = rpc_param->call_hdr.call_id;

	bk_err_t	ret_val;
	rpc_cmd_t	rpc_cmd;

	void * dst_buf = (void *)&xchg_buf->chnl_1_param[0];

	memcpy(dst_buf, rpc_param, param_len);

	rpc_cmd.chnl_hdr.data = 0;	/* clear hdr. */
	rpc_cmd.chnl_hdr.cmd  = RPC_CALL_CMD;
	rpc_cmd.cmd_param     = dst_buf;
	rpc_cmd.cmd_param_len = param_len;

	ret_val = mb_chnl_write(rpc_client_cb.chnl_id, (mb_chnl_cmd_t *)&rpc_cmd);
	if(ret_val != BK_OK)
	{
		rpc_client_cb.rpc_in_process  = 0;
		rtos_set_semaphore(&rpc_client_cb.chnl_sema);

		return ret_val;
	}

	if(rpc_param->call_hdr.ctrl & RPC_CTRL_NO_RETURN)
	{
		/* rtos_set_semaphore(&rpc_client_cb.chnl_sema) called by isr_callback. */

		return BK_OK;
	}

	ret_val = rtos_get_semaphore(&rpc_client_cb.api_rsp_sema, RPC_CALL_TIMEOUT);  /* isr_callback will set this semaphore. */
	if(ret_val != BK_OK)
	{
		rpc_client_cb.rpc_in_process  = 0;
		rtos_set_semaphore(&rpc_client_cb.chnl_sema);

		return ret_val;
	}

	rpc_client_cb.rpc_in_process  = 0;  /* must have been set to 0 by callback. */

	if(rpc_client_cb.rpc_call_failed)
	{
		rtos_set_semaphore(&rpc_client_cb.chnl_sema);
		return BK_FAIL;
	}

	if(ret_buf == NULL)
	{
		rtos_set_semaphore(&rpc_client_cb.chnl_sema);
		return BK_FAIL;
	}

	if(buf_len > rpc_client_cb.ret_buf_len)
		buf_len = rpc_client_cb.ret_buf_len;

	memcpy(ret_buf, rpc_client_cb.ret_buf, buf_len);

	rtos_set_semaphore(&rpc_client_cb.chnl_sema);

	return BK_OK;

}

bk_err_t client_send_simple_cmd(u32 sub_cmd, u32 param1, u32 param2)
{
	if(!rpc_client_cb.chnl_inited)
		return BK_FAIL;

	rtos_get_semaphore(&rpc_client_cb.chnl_sema, BEKEN_WAIT_FOREVER);

	bk_err_t		ret_val;
	mb_chnl_cmd_t	simple_cmd;

	simple_cmd.hdr.data = 0;	/* clear hdr. */
	simple_cmd.hdr.cmd  = SYS_CTRL_SIMPLE_CMD;

	simple_cmd.param1 = sub_cmd;
	simple_cmd.param2 = param1;
	simple_cmd.param3 = param2;

	ret_val = mb_chnl_write(rpc_client_cb.chnl_id, &simple_cmd);
	if(ret_val != BK_OK)
	{
		rtos_set_semaphore(&rpc_client_cb.chnl_sema);

		return ret_val;
	}

	/* rtos_set_semaphore(&rpc_client_cb.chnl_sema) will be called by tx_cmpl_isr. */

	return BK_OK;
}

/**    ============================    RPC client end  ============================   **/

#endif

#if CONFIG_MASTER_CORE

/**    ============================      RPC server    ============================   **/

typedef int (*rpc_svr_handler_t)(rpc_call_def_t * call_buf);

typedef struct
{
	u8		rpc_mod_id;
	rpc_svr_handler_t	svr_handler;
} rpc_svr_dispatcher_t;

typedef struct
{
	/* chnl data */
	u8					chnl_inited;
	u8					chnl_id;
	beken_semaphore_t	chnl_sema;

	/* server data */
	beken_semaphore_t	api_ind_sema;
	rpc_call_def_t	* 	call_buf;
	u8					rpc_in_process;
} rpc_server_cb_t;

static rpc_server_cb_t	rpc_server_cb; // = { .chnl_id = MB_CHNL_HW_CTRL, .chnl_inited = 0 };

static bk_err_t server_handle_simple_cmd(mb_chnl_cmd_t * cmd_buf)
{
	/* ack_buf is the cmd_buf.       */
	mb_chnl_ack_t * ack_buf = (mb_chnl_ack_t *)cmd_buf;

	/* must NOT change ack_buf->hdr. */

	BK_LOGI(MOD_TAG, "rpc cmd: %d, %d\r\n", cmd_buf->param2, cmd_buf->param3);

	switch(cmd_buf->param1)  /* sub cmd. */
	{
		case SIMPLE_TEST_CMD:
			ack_buf->ack_data2 += 1;
			ack_buf->ack_data3 += 1;
			break;

		case CPU1_POWER_UP:		// cpu1 indication, power up successfully.
			break;

		case CPU1_HEART_BEAT:	// cpu1 indication, alive indication.
			// contains the power save flag?
			break;

		case SET_SPINLOCK_GPIO:		// client announce spinlock addr.
			{
				// extern spinlock_t  *	gpio_spinlock_ptr;
				// gpio_spinlock_ptr = (spinlock_t *)(cmd_buf->param2);
				//    or
				// set_gpio_spinlock((spinlock_t *)(cmd_buf->param2));  // server save spinlock addr.
			}
			break;

		case GET_SPINLOCK_GPIO: 	// client get spinklock addr, return the addr in the first field following sub-cmd.
			{
				extern spinlock_t  	gpio_spinlock;
				ack_buf->ack_data2 = (u32)(&gpio_spinlock);
			}
			break;

		case GET_SPINLOCK_DMA:
			break;

		default:
			ack_buf->ack_data2 = (u32)(-1);
			ack_buf->ack_data3 = (u32)(-1);
			return BK_FAIL;
			break;
	}

	return BK_OK;
}

static void server_handle_simple_rsp(mb_chnl_ack_t * ack_buf)
{
	BK_LOGI(MOD_TAG, "rpc rsp: %d, %d\r\n", ack_buf->ack_data2, ack_buf->ack_data3);

	switch(ack_buf->ack_data1)  /* sub cmd. */
	{
		case SIMPLE_TEST_CMD:
			break;

		case GET_POWER_SAVE_FLAG:  // power save state returned from CPU1
			//if(ack_buf->ack_data2 != 0)
			//	set_cpu1_power_flag(PS_FLAG_CPU1);
			break;

		case GET_SPINLOCK_GPIO:  // server try to get spinlock address in client. client return the address, svr should save it.
			// gpio_spinlock_ptr = (spinlock_t *)ack_buf->ack_data2;
			break;

		default:
			break;
	}
}

static void rpc_server_rx_isr(rpc_server_cb_t * server_cb, mb_chnl_cmd_t *cmd_buf)
{
	u32		result = ACK_STATE_FAIL;
	mb_chnl_ack_t * ack_buf;

	if(cmd_buf->hdr.cmd == RPC_CALL_CMD)  /* call cmd from rpc client. */
	{
		rpc_cmd_t		* rpc_cmd = (rpc_cmd_t *)cmd_buf;

		server_cb->call_buf = (rpc_call_def_t *)rpc_cmd->cmd_param;

		if(server_cb->call_buf->call_hdr.data_len + sizeof(server_cb->call_buf->call_hdr) > rpc_cmd->cmd_param_len)
		{
			result = ACK_STATE_FAIL;
			goto rpc_server_rx_isr_exit;
		}
		BK_LOGI(MOD_TAG, "RPC_CALL_CMD: %d, %d, %d\r\n", cmd_buf->param1, cmd_buf->param2, cmd_buf->param3);

		result = ACK_STATE_PENDING;
		server_cb->rpc_in_process = 1;

		/* indicating to server task to process the call. */
		rtos_set_semaphore(&server_cb->api_ind_sema);

		goto rpc_server_rx_isr_exit;
	}

	if(cmd_buf->hdr.cmd == SYS_CTRL_SIMPLE_CMD)  /* simple cmd for IPC. */
	{
		server_handle_simple_cmd(cmd_buf);

		return;		/* don't change the ack_state of mb_chnl_ack_t, it is used as ack_data3. */
	}

	/*
	 *   !!!  FAULT  !!!
	 */
	BK_LOGE(MOD_TAG, "Fault in %s,cmd:%d\r\n", __func__, cmd_buf->hdr.cmd);

rpc_server_rx_isr_exit:

	/* overwrite the cmd_buf->param3 after the ISR handle complete.
	 * return the ack info to caller using the SAME buffer with cmd buffer.
	 *     !!!! [input as param / output as result ]  !!!!
	 */
	ack_buf = (mb_chnl_ack_t *)cmd_buf;
	ack_buf->ack_state = result;

	return;

}

static void rpc_server_tx_cmpl_isr(rpc_server_cb_t * server_cb, mb_chnl_ack_t *ack_buf)  /* tx_cmpl_isr */
{
	if(ack_buf->hdr.cmd == RPC_RET_CMD)
	{
		/* RPC_RET_CMD tx complete. */ /* nothing to do except releasing tx chnl. */
		rtos_set_semaphore(&server_cb->chnl_sema);

		return;
	}

	if(ack_buf->hdr.cmd == SYS_CTRL_SIMPLE_CMD)  /* simple cmd for IPC. */
	{
		server_handle_simple_rsp(ack_buf);
		rtos_set_semaphore(&server_cb->chnl_sema);
		return;
	}

	/*
	 *   !!!  FAULT  !!!
	 */
	BK_LOGE(MOD_TAG, "Fault in %s,cmd:%d\r\n", __func__, ack_buf->hdr.cmd);

	return;

}

static void rpc_svr_task( void *para );

bk_err_t rpc_server_init(void)
{
	bk_err_t		ret_code;
	beken_thread_t	thread_handle = NULL;

	u8 chnl_id = MB_CHNL_HW_CTRL;

	if(rpc_server_cb.chnl_inited)
		return BK_OK;

	memset(&rpc_server_cb, 0, sizeof(rpc_server_cb));
	rpc_server_cb.chnl_id = chnl_id;

	ret_code = rtos_init_semaphore_adv(&rpc_server_cb.chnl_sema, 1, 1);
	if(ret_code != BK_OK)
	{
		return ret_code;
	}

	ret_code = rtos_init_semaphore(&rpc_server_cb.api_ind_sema, 1);
	if(ret_code != BK_OK)
	{
		rtos_deinit_semaphore(&rpc_server_cb.chnl_sema);

		return ret_code;
	}

	ret_code = mb_chnl_open(chnl_id, &rpc_server_cb);
	if(ret_code != BK_OK)
	{
		rtos_deinit_semaphore(&rpc_server_cb.chnl_sema);
		rtos_deinit_semaphore(&rpc_server_cb.api_ind_sema);

		return ret_code;
	}

	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_RX_ISR, (void *)rpc_server_rx_isr);
	mb_chnl_ctrl(chnl_id, MB_CHNL_SET_TX_CMPL_ISR, (void *)rpc_server_tx_cmpl_isr);

	rpc_server_cb.chnl_inited = 1;

	ret_code = rtos_create_thread(&thread_handle,
							 BEKEN_DEFAULT_WORKER_PRIORITY, //BK_SYS_TASK_PRIO_7,
							 "rpc_svr",
							 (beken_thread_function_t)rpc_svr_task,
							 4096,
							 0);

	return BK_OK;
}

int rpc_server_listen_cmd(u32 timeout_ms)
{
	bk_err_t	ret_val;

	ret_val = rtos_get_semaphore(&rpc_server_cb.api_ind_sema, timeout_ms);

	if(ret_val == BK_OK)
		return 1;
	else
		return 0;
}

extern int bk_gpio_api_svr(rpc_call_def_t * call_buf);
extern int bk_dma_api_svr(rpc_call_def_t * call_buf);

static rpc_svr_dispatcher_t    svr_dispatcher[] =
{
	{RPC_MOD_GPIO,     bk_gpio_api_svr   },
	{RPC_MOD_DMA,      bk_dma_api_svr    },
};

void rpc_server_handle_cmd(void)
{
	int		rpc_handled = 0;
	int		i;

	if(rpc_server_cb.rpc_in_process == 0)  /* no rpc call cmd to handle. */
		return;

	for(i = 0; i < ARRAY_SIZE(svr_dispatcher); i++)
	{
		if(rpc_server_cb.call_buf->call_hdr.mod_id == svr_dispatcher[i].rpc_mod_id)
		{
			rpc_handled = svr_dispatcher[i].svr_handler(rpc_server_cb.call_buf);
		}
	}

	if(rpc_handled == 0)  /* rpc api not handled. */
	{
		rpc_ret_fail_t	rsp_buf;

		rsp_buf.call_hdr.call_id = rpc_server_cb.call_buf->call_hdr.call_id;
		rsp_buf.call_hdr.data_len = sizeof(rsp_buf) - sizeof(rsp_buf.call_hdr);
		rsp_buf.ret_val = BK_ERR_NOT_SUPPORT;

		rpc_server_rsp((rpc_ret_def_t *)&rsp_buf, sizeof(rsp_buf)); // return fail info.
	}

	return;
}

bk_err_t rpc_server_rsp(rpc_ret_def_t *rsp_param, u16 param_len)
{
	if(!rpc_server_cb.chnl_inited)
		return BK_FAIL;

	if(rpc_server_cb.rpc_in_process == 0)
		return BK_FAIL;

	rtos_get_semaphore(&rpc_server_cb.chnl_sema, BEKEN_WAIT_FOREVER);

	bk_err_t	ret_val;
	rpc_cmd_t	rpc_cmd;

	void * dst_buf = (void *)&xchg_buf->chnl_1_ret[0];

	if(param_len > RPC_XCHG_DATA_MAX)
		param_len = RPC_XCHG_DATA_MAX;

	memcpy(dst_buf, rsp_param, param_len);

	rpc_cmd.chnl_hdr.data = 0;	/* clear hdr. */
	rpc_cmd.chnl_hdr.cmd  = RPC_RET_CMD;
	rpc_cmd.cmd_param     = dst_buf;
	rpc_cmd.cmd_param_len = param_len;

	ret_val = mb_chnl_write(rpc_server_cb.chnl_id, (mb_chnl_cmd_t *)&rpc_cmd);

	rpc_server_cb.rpc_in_process  = 0;

	if(ret_val != BK_OK)
	{
		rtos_set_semaphore(&rpc_server_cb.chnl_sema);

		return ret_val;
	}

	/* rtos_set_semaphore(&rpc_server_cb.chnl_sema) called by tx_cmpl_isr. */

	return BK_OK;

}

bk_err_t server_send_simple_cmd(u32 sub_cmd, u32 param1, u32 param2)
{
	if(!rpc_server_cb.chnl_inited)
		return BK_FAIL;

	rtos_get_semaphore(&rpc_server_cb.chnl_sema, BEKEN_WAIT_FOREVER);

	bk_err_t		ret_val;
	mb_chnl_cmd_t	simple_cmd;

	simple_cmd.hdr.data = 0;	/* clear hdr. */
	simple_cmd.hdr.cmd  = SYS_CTRL_SIMPLE_CMD;

	simple_cmd.param1 = sub_cmd;
	simple_cmd.param2 = param1;
	simple_cmd.param3 = param2;

	ret_val = mb_chnl_write(rpc_server_cb.chnl_id, &simple_cmd);
	if(ret_val != BK_OK)
	{
		rtos_set_semaphore(&rpc_server_cb.chnl_sema);

		return ret_val;
	}

	/* rtos_set_semaphore(&rpc_server_cb.chnl_sema) will be called by tx_cmpl_isr. */

	return BK_OK;
}

static void rpc_svr_task( void *para )
{
	// rpc_server_init

	while(1)
	{
		rpc_server_listen_cmd(BEKEN_WAIT_FOREVER);
		rpc_server_handle_cmd();
	}
}

/**    ============================    RPC server end  ============================   **/

#endif

