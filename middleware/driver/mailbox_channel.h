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

#ifndef _mailbox_channel_h_
#define _mailbox_channel_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_typedef.h"

enum
{
	MB_CHNL_HW_CTRL = 0,
	MB_CHNL_LOG     = 1,
	MB_LOG_CHNL_NUM,
};

typedef union
{
	struct
	{
		u32		cmd           :  15;
		u32		com_fail      :  1;		/* cmd NO handler in peer CPU, it is an ACK bit from peer CPU. */
		u32		Reserved      :  16;	/* reserved for system. */
	} ;
	u32		data;
} mb_chnnl_hdr_t;

typedef struct
{
	mb_chnnl_hdr_t	hdr;

	u32		param1;
	u32		param2;
	u32		param3;
} mb_chnl_cmd_t;

enum
{
	ACK_STATE_PENDING = 0x01,		/* cmd is being handled in the peer CPU. */
	ACK_STATE_COMPLETE, 			/* cmd handling is competed, addtional infos in ack_data1&ack_data2. */
	ACK_STATE_FAIL, 				/* cmd failed, addtional infos in ack_data1&ack_data2. */
};

typedef struct
{
	mb_chnnl_hdr_t	hdr;

	u32		ack_data1;
	u32		ack_data2;
	u32		ack_state;				/* ack_state or ack_data3, depends on applications. */
} mb_chnl_ack_t;

typedef union
{
	mb_chnl_cmd_t	cmd;
	mb_chnl_ack_t	ack;
} mb_chnl_buff_t;

typedef void  (* chnl_tx_cmpl_isr_t)(u8 log_chnl, mb_chnl_ack_t *ack_buf);
typedef void  (* chnl_tx_isr_t)(u8 log_chnl);

/*
 *  rx_isr firstly use the buf as the command buffer, after handled the cmd, 
 *  then use the buf as acknowledge buffer to fill the handle result.
 */
typedef void  (* chnl_rx_isr_t)(u8 log_chnl, mb_chnl_cmd_t *cmd_buf);

enum
{
	MB_CHNL_GET_STATUS = 0,
	MB_CHNL_SET_RX_ISR,
	MB_CHNL_SET_TX_ISR,
	MB_CHNL_SET_TX_CMPL_ISR,
};

/*
typedef struct
{
	u8    chnl_busy;
} mb_chnl_get_status_param_t;

typedef struct
{
	void * rx_isr;
} mb_chnl_set_rx_isr_param_t;

typedef struct
{
	void * tx_isr;
} mb_chnl_set_tx_isr_param_t;

typedef struct
{
	void * tx_cmpl_isr;
} mb_chnl_set_tx_cmpl_isr_param_t;
*/

/*
  * init logical chnanel module.
  * return:
  *     succeed: BK_OK;
  *     failed  : fail code.
  *
  */
bk_err_t mb_chnl_init(void);

/*
  * open logical chnanel.
  * input:
  *     log_chnl  : logical channel id to open.
  * return:
  *     succeed: BK_OK;
  *     failed  : fail code.
  *
  */
bk_err_t mb_chnl_open(u8 log_chnl);

/*
  * close logical chnanel.
  * input:
  *     log_chnl  : logical channel id to close.
  * return:
  *     succeed: BK_OK;
  *     failed  : fail code.
  *
  */
bk_err_t mb_chnl_close(u8 log_chnl);

/*
  * read from logical chnanel.
  * input:
  *     log_chnl     : logical channel id to read.
  *     read_buf       : buffer to receive channel cmd data.
  * return:
  *     succeed: BK_OK;
  *     failed  : fail code.
  *
  */
bk_err_t mb_chnl_read(u8 log_chnl, mb_chnl_cmd_t * read_buf);

/*
  * write to logical chnanel.
  * input:
  *     log_chnl     : logical channel id to write.
  *     cmd_buf       : command buffer to send.
  * 
  * return:
  *     succeed: BK_OK;
  *     failed  : fail code.
  *
  */
bk_err_t mb_chnl_write(u8 log_chnl, mb_chnl_cmd_t * cmd_buf);

/*
  * logical chnanel misc io (set/get param).
  * input:
  *     log_chnl     : logical channel id to set/get param.
  *     cmd          : control command for logical channel.
  *     param      :  parameter of the command.
  * return:
  *     succeed: BK_OK;
  *     failed  : fail code.
  *
  */
bk_err_t mb_chnl_ctrl(u8 log_chnl, u8 cmd, void * param);


#ifdef __cplusplus
}
#endif

#endif /* _mailbox_channel_h_ */

