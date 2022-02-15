
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cli.h"
#include "rtos_pub.h"
#include "shell_drv.h"


#define SHELL_EVENT_TX_REQ  0x01
#define SHELL_EVENT_RX_IND  0x02

#define SHELL_LOG_BUF1_LEN      128
#define SHELL_LOG_BUF2_LEN      64
#define SHELL_LOG_BUF3_LEN      32

#ifdef CONFIG_SLAVE_CORE
#define SHELL_LOG_BUF1_NUM      16
#define SHELL_LOG_BUF2_NUM      24
#define SHELL_LOG_BUF3_NUM      16
#else
#define SHELL_LOG_BUF1_NUM      32
#define SHELL_LOG_BUF2_NUM      48
#define SHELL_LOG_BUF3_NUM      32
#endif

#define SHELL_LOG_BUF_NUM       (SHELL_LOG_BUF1_NUM + SHELL_LOG_BUF2_NUM + SHELL_LOG_BUF3_NUM)

#define SHELL_LOG_PEND_NUM	(SHELL_LOG_BUF_NUM + 2)  /* 1: for RSP, 1: reserved. */
#define SHELL_LOG_BUSY_NUM	(SHELL_LOG_PEND_NUM)         /* depending on lower driver's pending queue size. */

#define SHELL_ASSERT_BUF_LEN	140
#define SHELL_CMD_BUF_LEN		140
#define SHELL_RSP_BUF_LEN		1024

#define SHELL_RSP_QUEUE_ID	    (7)
#define SHELL_FW_QUE_ID         (8)

#define MAX_TRACE_ARGS      10
#define MOD_NAME_LEN        4

#define HEX_SYNC_CHAR       0xFE
#define HEX_MOD_CHAR        0xFF
#define HEX_ESC_CHAR        0xFD

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

#define TBL_SIZE(tbl)		(sizeof(tbl) / sizeof(tbl[0]))

enum
{
	CMD_TYPE_TEXT = 0,
	CMD_TYPE_HEX,
	CMD_TYPE_BKREG,   /* patch for BK_REG tool cmd. */
	CMD_TYPE_INVALID,
};

/* patch for BK_REG tool. */
enum
{
	BKREG_WAIT_01 = 0,
	BKREG_WAIT_E0,
	BKREG_WAIT_FC,
};

typedef struct
{
	u8     rsp_buff[SHELL_RSP_BUF_LEN];
	bool_t   rsp_ongoing;
	
	u8     cur_cmd_type;
	u8     cmd_buff[SHELL_CMD_BUF_LEN];
	u8     cmd_data_len;

	/* patch for BK_REG tool. */
	/* added one state machine for BK_REG tool cmd. */
	u8     bkreg_state;
	u8     bkreg_left_byte;
	/* patch end. */

	u8     assert_buff[SHELL_ASSERT_BUF_LEN];
	u8     assert_data_len;

	u8     log_level;
	bool_t   echo_enable;
} cmd_line_t;

#define GET_BLOCK_ID(blocktag)          ((blocktag) & 0xFF)
#define GET_QUEUE_ID(blocktag)          (((blocktag) & 0x0F00) >> 8)
#define MAKE_BLOCK_TAG(blk_id, q_id)    (((blk_id) & 0xFF) | (((q_id) & 0x0F) << 8) )

typedef struct
{
	u16     blk_tag;        /* bit0~bit7: blk_id,    bit8~bit11: queue_id; */
	u16     packet_len;
} tx_packet_t;

typedef struct
{
	tx_packet_t     packet_list[SHELL_LOG_PEND_NUM];
	u16     list_out_idx;
	u16     list_in_idx;
} pending_queue_t;

typedef struct
{
	u16     blk_list[SHELL_LOG_BUSY_NUM];
	u16     list_out_idx;
	u16     list_in_idx;
} busy_queue_t;

typedef struct
{
	u8   *  const  log_buf;
	u16   * const  blk_list;
	const u16      blk_num;
	const u16      blk_len;
	u16     list_out_idx;
	u16     list_in_idx;
	u16     free_blk_num;
	u32     empty_cnt;
} free_queue_t;

static u8    shell_log_buff1[SHELL_LOG_BUF1_NUM * SHELL_LOG_BUF1_LEN];
static u8    shell_log_buff2[SHELL_LOG_BUF2_NUM * SHELL_LOG_BUF2_LEN];
static u8    shell_log_buff3[SHELL_LOG_BUF3_NUM * SHELL_LOG_BUF3_LEN];
static u16   buff1_free_list[SHELL_LOG_BUF1_NUM];
static u16   buff2_free_list[SHELL_LOG_BUF2_NUM];
static u16   buff3_free_list[SHELL_LOG_BUF3_NUM];

/*    queue sort ascending in blk_len.    */
static free_queue_t       free_queue[3] =
	{
		{.log_buf = shell_log_buff3, .blk_list = buff3_free_list, .blk_num = SHELL_LOG_BUF3_NUM, \
			.blk_len = SHELL_LOG_BUF3_LEN, .list_out_idx = 0, .list_in_idx = 0, \
			.free_blk_num = SHELL_LOG_BUF3_NUM, .empty_cnt = 0},

		{.log_buf = shell_log_buff2, .blk_list = buff2_free_list, .blk_num = SHELL_LOG_BUF2_NUM, \
			.blk_len = SHELL_LOG_BUF2_LEN, .list_out_idx = 0, .list_in_idx = 0, \
			.free_blk_num = SHELL_LOG_BUF2_NUM, .empty_cnt = 0},

		{.log_buf = shell_log_buff1, .blk_list = buff1_free_list, .blk_num = SHELL_LOG_BUF1_NUM, \
			.blk_len = SHELL_LOG_BUF1_LEN, .list_out_idx = 0, .list_in_idx = 0, \
			.free_blk_num = SHELL_LOG_BUF1_NUM, .empty_cnt = 0},
	};

static busy_queue_t       log_busy_queue;
static pending_queue_t    pending_queue;

static cmd_line_t  cmd_line_buf;

#ifdef CONFIG_SLAVE_CORE
static shell_dev_t * log_dev = &shell_dev_mb;
static shell_dev_t * cmd_dev = &shell_dev_mb;
#else

static shell_dev_t * log_dev = &shell_uart;
static shell_dev_t * cmd_dev = &shell_uart;

#ifdef CONFIG_MASTER_CORE
static shell_dev_ipc_t * ipc_dev = &shell_dev_ipc;
#endif

#endif

static bool_t shell_init_ok = bFALSE;

beken_semaphore_t   shell_semaphore;  // will release from ISR.


static bool_t create_shell_event(void)
{
	rtos_init_semaphore(&shell_semaphore, 1);

	return bTRUE;
}

static bool_t set_shell_event(u32 event_flag)
{
	(void)event_flag;

	rtos_set_semaphore(&shell_semaphore);

	return bTRUE;
}

static u32 wait_any_event(u32 timeout)
{
	int result;

	result = rtos_get_semaphore(&shell_semaphore, timeout);

	if(result == kTimeoutErr)
		return 0;

	return SHELL_EVENT_RX_IND;
}


static void tx_req_process(void);

#if 0
static u16 get_free_buff_cnt(free_queue_t *free_q)
{
	u16    free_cnt;

	if(free_q->list_out_idx >= free_q->list_in_idx)
	{
		free_cnt = free_q->blk_num + free_q->list_in_idx - free_q->list_out_idx;
	}
	else
	{
		free_cnt = free_q->list_in_idx - free_q->list_out_idx;
	}

	return free_cnt; /* should reserve 1 block. *//* (list_out_idx == list_in_idx) means all buf free. */
}
#endif

static u8 * alloc_log_blk(u16 log_len, u16 *blk_tag)
{
	u16    free_blk_id;
	u8     queue_id;
	free_queue_t * free_q;
	u8     *       blk_buf = NULL;

	//disable_interrupt(); // called from task context, use semaphore instead of locking interrupt.
	//get_shell_mutex();
	u32  int_mask = rtos_disable_int();

	for(queue_id = 0; queue_id < TBL_SIZE(free_queue); queue_id++)
	{
		free_q = &free_queue[queue_id];

		/*    queue ascending in blk_len.    */
		if(free_q->blk_len < log_len)
			continue;

		if(/*get_free_buff_cnt(free_q) */ free_q->free_blk_num > 0) 
		{
			free_blk_id = free_q->blk_list[free_q->list_out_idx];
			free_q->list_out_idx = (free_q->list_out_idx + 1) % free_q->blk_num;
			free_q->free_blk_num--;

			blk_buf = &free_q->log_buf[free_blk_id * free_q->blk_len];
			*blk_tag = MAKE_BLOCK_TAG(free_blk_id, queue_id);

			break;
		}
		else
		{
			free_q->empty_cnt++;
		}
	}

	//enable_interrupt(); // called from task context, use semaphore instead of locking interrupt.
	//release_shell_mutex();
	rtos_enable_int(int_mask);

	return blk_buf;
}

static bool_t free_log_blk(u16 block_tag)
{
	u8       queue_id = GET_QUEUE_ID(block_tag);
	u16     blk_id = GET_BLOCK_ID(block_tag);
	free_queue_t *free_q;

	if(queue_id >= TBL_SIZE(free_queue))
		return bFALSE;

	free_q = &free_queue[queue_id];

	if(blk_id >= free_q->blk_num)
		return bFALSE;

	//disable_interrupt(); // called from tx-complete only, don't lock interrupt.

	free_q->blk_list[free_q->list_in_idx] = blk_id;
	free_q->list_in_idx = (free_q->list_in_idx + 1) % free_q->blk_num;
	free_q->free_blk_num++;

	//enable_interrupt(); // called from tx-complete only, don't lock interrupt.

	return bTRUE;
}

static void push_pending_queue(u16 blk_tag, u16 data_len)
{
	//get_shell_mutex();
	
	pending_queue.packet_list[pending_queue.list_in_idx].blk_tag = blk_tag;
	pending_queue.packet_list[pending_queue.list_in_idx].packet_len = data_len;
	
	pending_queue.list_in_idx = (pending_queue.list_in_idx + 1) % SHELL_LOG_PEND_NUM;

	//release_shell_mutex();

	return;
}

static void pull_pending_queue(u16 *blk_tag, u16 *data_len)
{
	*blk_tag     = pending_queue.packet_list[pending_queue.list_out_idx].blk_tag;
	*data_len   = pending_queue.packet_list[pending_queue.list_out_idx].packet_len;

	pending_queue.list_out_idx = (pending_queue.list_out_idx + 1) % SHELL_LOG_PEND_NUM;

	return;
}

int shell_assert_out(bool bContinue, char * format, ...);

/* call from TX ISR. */
static void shell_tx_complete(u8 *pbuf, u16 buf_tag)
{
	u16     block_tag;
	u8      queue_id = GET_QUEUE_ID(buf_tag);
	u16     blk_id = GET_BLOCK_ID(buf_tag);
	free_queue_t *free_q;

	/* rsp ok ?? */
	if( queue_id == SHELL_RSP_QUEUE_ID )    /* rsp. */
	{
		/* it is called from cmd_dev tx ISR. */

		if ( (pbuf != cmd_line_buf.rsp_buff) || (blk_id != 0) || 
			( !cmd_line_buf.rsp_ongoing ) )
		{
			/* something wrong!!! */
			shell_assert_out(bTRUE, "FAULT: in rsp.\r\n");
		}

		cmd_line_buf.rsp_ongoing = bFALSE;   /* rsp compelete, rsp_buff can be used for next cmd/response. */

		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
		tx_req_process();

		return;  
	}

	if (queue_id < TBL_SIZE(free_queue))   /* from log busy queue. */
	{
		/* it is called from log_dev tx ISR. */

		free_q = &free_queue[queue_id];

		block_tag = log_busy_queue.blk_list[log_busy_queue.list_out_idx];

		if( ( buf_tag != block_tag ) || (blk_id >= free_q->blk_num) || 
			( (&free_q->log_buf[blk_id * free_q->blk_len]) != pbuf) )
		{
			/* something wrong!!! */
			/*        FAULT !!!!      */
			shell_assert_out(bTRUE, "FATAL:%x,%x\r\n", buf_tag, block_tag);

			return;
		}

		/* de-queue from busy queue. */
		log_busy_queue.list_out_idx = (log_busy_queue.list_out_idx + 1) % SHELL_LOG_BUSY_NUM;

		/* free buffer to queue. */
		free_log_blk(block_tag);

		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
		tx_req_process();

		return;
	}

	/*        FAULT !!!!      */
	shell_assert_out(bTRUE, "FATAL:%x,\r\n", buf_tag);

	return;
}

/* call from RX ISR. */
static void shell_rx_indicate(void)
{
	set_shell_event(SHELL_EVENT_RX_IND);

	return;
}

static u16 append_trace_data(u8 * trace_buf, u16 buf_len, u32 trace_data)
{
	u16   cnt = 0, i;
	u8  * data_ptr = (u8 *)&trace_data;

	for(i = 0; i < sizeof(trace_data); i++)
	{
		if( (*data_ptr == HEX_SYNC_CHAR) || 
			(*data_ptr == HEX_ESC_CHAR) )
		{
			if(cnt < (buf_len - 1))
			{
				trace_buf[cnt++] = HEX_ESC_CHAR;
				trace_buf[cnt++] = (*data_ptr) ^ HEX_MOD_CHAR;
			}
		}
		else
		{
			if(cnt < buf_len)
			{
				trace_buf[cnt++] = (*data_ptr);
			}
		}

		data_ptr++;
	}

	return cnt;
}

static bool_t echo_out(u8 * echo_str, u16 len)
{
	u16	 wr_cnt;

	if(len == 0)
		return bTRUE;
	
	wr_cnt = cmd_dev->dev_drv->write_echo(cmd_dev, echo_str, len);

	return (wr_cnt == len);
}

/*    NOTICE:  this can only be called by shell task internally (cmd handler). */
/*             it is not re-enterance function. */
static bool_t rsp_out(u8 * rsp_msg, u16 msg_len)
{
	u16    rsp_blk_tag = MAKE_BLOCK_TAG(0, SHELL_RSP_QUEUE_ID);

	if(rsp_msg != cmd_line_buf.rsp_buff)
	{
		if(msg_len > sizeof(cmd_line_buf.rsp_buff))
		{
			msg_len = sizeof(cmd_line_buf.rsp_buff);;
		}

		memcpy(cmd_line_buf.rsp_buff, rsp_msg, msg_len);
	}

	if(log_dev != cmd_dev)
	{
		// set TRUE, then call driver, in case TX_COMPLETE_ISR will happen before set TRUE.
		cmd_line_buf.rsp_ongoing = bTRUE;  // one by one for cmd handler.

		/* dedicated device for cmd/response, don't enqueue the msg to pending queue. */
		/* send to cmd dev directly. */
		cmd_dev->dev_drv->write_async(cmd_dev, cmd_line_buf.rsp_buff, msg_len, rsp_blk_tag); 
	}
	else
	{
		/* shared device for response & log, push the rsp msg to pending queue. */

		u32  int_mask = rtos_disable_int();

		cmd_line_buf.rsp_ongoing = bTRUE;  // one by one for cmd handler. set to true before trigger the TX.

		push_pending_queue(rsp_blk_tag, msg_len);

		//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
		tx_req_process();

		rtos_enable_int(int_mask);
	}

	return bTRUE;
}

/* call this in !* DISABLE *! interrupt context. */
static void tx_req_process(void)
{
	u8		*packet_buf = NULL;
	u16		block_tag;
	u16		log_len;
	u16		tx_ready;
	u16		blk_id;
	u8		queue_id;
	free_queue_t *free_q;

	/* maybe tx_req is from tx_complete_callback, check if there any log in queue. */
	if(pending_queue.list_out_idx == pending_queue.list_in_idx)  /* queue empty! */
		return;
	
	tx_ready = 0;
	
	log_dev->dev_drv->io_ctrl(log_dev, SHELL_IO_CTRL_GET_STATUS, &tx_ready);

	if(tx_ready == 0)
		return;

	/**    ====     POP from pending queue     ====    **/
	pull_pending_queue(&block_tag, &log_len);

	queue_id = GET_QUEUE_ID(block_tag);
	blk_id = GET_BLOCK_ID(block_tag);

	if (queue_id < TBL_SIZE(free_queue))
	{
		free_q = &free_queue[queue_id];

		if(blk_id < free_q->blk_num)
		{
			packet_buf = &free_q->log_buf[blk_id * free_q->blk_len];
		}
	}
	else if(queue_id == SHELL_RSP_QUEUE_ID)
	{
		packet_buf = cmd_line_buf.rsp_buff;

		if((cmd_line_buf.rsp_ongoing == bFALSE) || (log_dev != cmd_dev) || (blk_id != 0))
		{
			shell_assert_out(bTRUE, "xFATAL: in Tx_req\r\n");
			/*		  FAULT !!!!	  */
			/* if log_dev is not the same with cmd_dev,
			 * rsp will not be pushed into pending queue.
			 */
		}
	}
	else
	{
		/*		  FAULT !!!!	  */
		shell_assert_out(bTRUE, "xFATAL: in Tx_req %x.\r\n", block_tag);
	}

	if(packet_buf == NULL)
		return;

	/* rsp buff not enqueue the busy-queue. */
	if(queue_id < TBL_SIZE(free_queue))   // (queue_id != SHELL_RSP_QUEUE_ID)
	{
		log_busy_queue.blk_list[log_busy_queue.list_in_idx] = block_tag;
		log_busy_queue.list_in_idx = (log_busy_queue.list_in_idx + 1) % SHELL_LOG_BUSY_NUM;
	}

	log_dev->dev_drv->write_async(log_dev, packet_buf, log_len, block_tag); /* send to log dev driver. */
	/* if driver return 0, should free log-block or not de-queue pending queue and try again. */
	/* if return 1, push log-block into busy queue is OK. */

	return;
}

int shell_trace_out( u32 trace_id, ... );

static void rx_ind_process(void)
{
	u16   read_cnt, buf_len, echo_len;
	u16   i = 0;
	bool_t  cmd_rx_done = bFALSE, need_backspace = bFALSE;
	
	if(cmd_line_buf.rsp_ongoing)
	{
		/* previous cmd not complete, discard new cmds. */
		while(bTRUE)
		{
			read_cnt = cmd_dev->dev_drv->read(cmd_dev, cmd_line_buf.assert_buff, sizeof(cmd_line_buf.assert_buff));

			if(read_cnt < sizeof(cmd_line_buf.assert_buff))
				break;
		}

		return;
	}

	/* cmd_line_buf.rsp_buff is free, can used for read buff temporarily  */

	buf_len = MIN(SHELL_CMD_BUF_LEN, SHELL_RSP_BUF_LEN);

	while(bTRUE)
	{
		read_cnt = cmd_dev->dev_drv->read(cmd_dev, cmd_line_buf.rsp_buff, buf_len);

		echo_len = 0;

		for(i = 0; i < read_cnt; i++)
		{
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_INVALID)
			{
				if(cmd_line_buf.rsp_buff[i] == HEX_SYNC_CHAR)  // SYNC_CHAR, hex frame start.
				{
					cmd_line_buf.cur_cmd_type = CMD_TYPE_HEX;

					cmd_line_buf.cmd_data_len = 0;
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = HEX_SYNC_CHAR;
					cmd_line_buf.cmd_data_len++;
						
					continue;
				}

				echo_len++;          /* SYNC_CHAR not echo. */

				if((cmd_line_buf.rsp_buff[i] >= 0x20) && (cmd_line_buf.rsp_buff[i] < 0x7f))
				{
					cmd_line_buf.cur_cmd_type = CMD_TYPE_TEXT;

					cmd_line_buf.cmd_data_len = 0;
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = cmd_line_buf.rsp_buff[i];
					cmd_line_buf.cmd_data_len++;
						
					continue;
				}

				/* patch for BK_REG tool. */
				if(cmd_line_buf.bkreg_state == BKREG_WAIT_01)
				{
					if(cmd_line_buf.rsp_buff[i] == 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_E0;
				}
				else if(cmd_line_buf.bkreg_state == BKREG_WAIT_E0)
				{
					if(cmd_line_buf.rsp_buff[i] == 0xE0)
						cmd_line_buf.bkreg_state = BKREG_WAIT_FC;
					else if(cmd_line_buf.rsp_buff[i] != 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_01;
				}
				else if(cmd_line_buf.bkreg_state == BKREG_WAIT_FC)
				{
					if(cmd_line_buf.rsp_buff[i] == 0xFC)
					{
						cmd_line_buf.cur_cmd_type = CMD_TYPE_BKREG;

						cmd_line_buf.cmd_buff[0] = 0x01;
						cmd_line_buf.cmd_buff[1] = 0xE0;
						cmd_line_buf.cmd_buff[2] = 0xFC;

						cmd_line_buf.cmd_data_len = 3;

						echo_len = 0;   // cann't echo anything.

						continue;
					}
					else if(cmd_line_buf.rsp_buff[i] != 0x01)
						cmd_line_buf.bkreg_state = BKREG_WAIT_01;
					else
						cmd_line_buf.bkreg_state = BKREG_WAIT_E0;
				}

			}

			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_HEX)
			{
				if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
				{
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = cmd_line_buf.rsp_buff[i];
					cmd_line_buf.cmd_data_len++;
				}

				if(cmd_line_buf.rsp_buff[i] == HEX_SYNC_CHAR)  // SYNC_CHAR, hex frame end.
				{
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len - 1] = HEX_SYNC_CHAR;  // in case cmd_data_len overflow.
					cmd_rx_done = bTRUE;
					break;
				}
			}

			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_TEXT)
			{
				echo_len++;
				if(cmd_line_buf.rsp_buff[i] == '\b')
				{
					if(cmd_line_buf.cmd_data_len > 0)
					{
						cmd_line_buf.cmd_data_len--;

						if(cmd_line_buf.cmd_data_len == 0)
							need_backspace = bTRUE;
					}
				}
				else if((cmd_line_buf.rsp_buff[i] == '\n') || (cmd_line_buf.rsp_buff[i] == '\r'))
				{
					if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
					{
						cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = 0;
					}
					else
					{
						cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len - 1] = 0;  // in case cmd_data_len overflow.
					}
					
					cmd_rx_done = bTRUE;
					break;
				}
				else if((cmd_line_buf.rsp_buff[i] >= 0x20))
				{
					if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
					{
						cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = cmd_line_buf.rsp_buff[i];
						cmd_line_buf.cmd_data_len++;
					}
				}

			}

			/* patch for BK_REG tool. */
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
			{
				echo_len = 0;   // cann't echo anything.

				/* p[0] = 0x1, p[1]=0xe0, p[2]=0xfc, p[3]=len. */
				if(cmd_line_buf.cmd_data_len == 3)
				{
					cmd_line_buf.bkreg_left_byte = cmd_line_buf.rsp_buff[i] + 1;  // +1, because will -1 in next process.

					if((cmd_line_buf.bkreg_left_byte + 3) >= sizeof(cmd_line_buf.cmd_buff))  // 3 bytes of header + 1 byte of len.
					{
						cmd_line_buf.cmd_data_len = 0;

						cmd_rx_done = bTRUE;
						break;
					}
				}

				if(cmd_line_buf.cmd_data_len < sizeof(cmd_line_buf.cmd_buff))
				{
					cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len] = cmd_line_buf.rsp_buff[i];
					cmd_line_buf.cmd_data_len++;
				}

				cmd_line_buf.bkreg_left_byte--;

				if(cmd_line_buf.bkreg_left_byte == 0)
				{
					cmd_rx_done = bTRUE;
					break;
				}
			}
		}

		if( cmd_rx_done )
		{
			/* patch for BK_REG tool. */
			if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
			{
				break;  // cann't echo anything.
			}

			if(cmd_line_buf.echo_enable)
			{
				echo_out(&cmd_line_buf.rsp_buff[0], echo_len);
				echo_out((u8 *)"\r\n", 2);
			}

			break;
		}
		else
		{
			/* patch for BK_REG tool. */
			if( (cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG) ||
				((cmd_line_buf.cur_cmd_type == CMD_TYPE_INVALID) && (cmd_line_buf.bkreg_state != BKREG_WAIT_01)) )
			{
				 // cann't echo anything.
			}
			else if(cmd_line_buf.echo_enable)
			{
				if(echo_len > 0)
				{
					if( (cmd_line_buf.rsp_buff[echo_len - 1] == '\b') || 
						(cmd_line_buf.rsp_buff[echo_len - 1] == 0x7f) ) /* DEL */
					{
						echo_len--;
						if((cmd_line_buf.cmd_data_len > 0) || need_backspace)
							echo_out((u8 *)"\b \b", 3);
					}
					
					u8    cr_lf = 0;

					if(echo_len == 1)
					{
						if( (cmd_line_buf.rsp_buff[echo_len - 1] == '\r') || 
							(cmd_line_buf.rsp_buff[echo_len - 1] == '\n') )
						{
							cr_lf = 1;
						}
					}
					else if(echo_len == 2)
					{
						if( (memcmp(cmd_line_buf.rsp_buff, "\r\n", 2) == 0) || 
							(memcmp(cmd_line_buf.rsp_buff, "\n\r", 2) == 0) )
						{
							cr_lf = 1;
						}
					}

					if(cr_lf != 0)
					{
						echo_out((u8 *)"\r\n# ", 3);
						echo_len = 0;
					}
				}
				echo_out(cmd_line_buf.rsp_buff, echo_len);
			}
		}

		if(read_cnt < buf_len) /* all data are read out. */
			break;
	}

	/* can re-use *buf_len*. */
	if( cmd_rx_done )
	{
		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_HEX)
		{
			#if 1   /* it's test code. */
			buf_len = sprintf((char *)&cmd_line_buf.rsp_buff[0], "\r\nHex count: %d, ", cmd_line_buf.cmd_data_len);
			buf_len += sprintf((char *)&cmd_line_buf.rsp_buff[buf_len], "Hdr: %x, Tail: %x.\r\n", \
				           cmd_line_buf.cmd_buff[0], cmd_line_buf.cmd_buff[cmd_line_buf.cmd_data_len - 1]);
			
			for(i = 1; (i < cmd_line_buf.cmd_data_len) && (i < 18); i++)
			{
				buf_len += sprintf((char *)&cmd_line_buf.rsp_buff[buf_len], "%x ", cmd_line_buf.cmd_buff[i]);
			}

			strcpy((char *)&cmd_line_buf.rsp_buff[buf_len], "\r\n#");
			rsp_out(cmd_line_buf.rsp_buff, strlen((char *)cmd_line_buf.rsp_buff));

			shell_trace_out(0x39, 8, cmd_line_buf.cmd_buff[0], cmd_line_buf.cmd_buff[1], cmd_line_buf.cmd_buff[2], cmd_line_buf.cmd_buff[3],  \
				cmd_line_buf.cmd_buff[4], cmd_line_buf.cmd_buff[5], cmd_line_buf.cmd_buff[6], cmd_line_buf.cmd_buff[7]);

			#endif
		}

		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_TEXT)
		{
			cmd_line_buf.rsp_buff[0] = 0;
			/* handle command. */
			if( cmd_line_buf.cmd_data_len > 0 )
				handle_shell_input( (char *)cmd_line_buf.cmd_buff, cmd_line_buf.cmd_data_len, (char *)cmd_line_buf.rsp_buff, sizeof(cmd_line_buf.rsp_buff) );

			cmd_line_buf.rsp_buff[SHELL_RSP_BUF_LEN - 4] = 0;

			buf_len = strlen((char *)cmd_line_buf.rsp_buff);
			if(buf_len > (SHELL_RSP_BUF_LEN - 4))
				buf_len = (SHELL_RSP_BUF_LEN - 4);
			buf_len += sprintf((char *)&cmd_line_buf.rsp_buff[buf_len], "\r\n#");

			rsp_out(cmd_line_buf.rsp_buff, buf_len);
		}

		/* patch for BK_REG tool. */
		if(cmd_line_buf.cur_cmd_type == CMD_TYPE_BKREG)
		{
			if(cmd_line_buf.cmd_data_len > 3)
			{
#if CONFIG_BKREG
				extern int bkreg_run_command(const char *cmd, int flag);

				bkreg_run_command((const char *)&cmd_line_buf.cmd_buff[0], (int)cmd_line_buf.cmd_data_len);
#endif // CONFIG_BKREG
			}
		}

		cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;  /* reset cmd line to interpret new cmd. */
		cmd_line_buf.cmd_data_len = 0;
		cmd_line_buf.bkreg_state = BKREG_WAIT_01;	/* reset state machine. */
	}

	return;
}

static void shell_task_init(void)
{
	u16		i;

	for(i = 0; i < SHELL_LOG_BUF1_NUM; i++)
	{
		buff1_free_list[i] = i;
	}
	for(i = 0; i < SHELL_LOG_BUF2_NUM; i++)
	{
		buff2_free_list[i] = i;
	}
	for(i = 0; i < SHELL_LOG_BUF3_NUM; i++)
	{
		buff3_free_list[i] = i;
	}

	memset(&log_busy_queue, 0, sizeof(log_busy_queue));
	memset(&pending_queue, 0, sizeof(pending_queue));

	cmd_line_buf.rsp_ongoing = bFALSE;
	cmd_line_buf.cur_cmd_type = CMD_TYPE_INVALID;
	cmd_line_buf.cmd_data_len = 0;
	cmd_line_buf.bkreg_state = BKREG_WAIT_01;
	cmd_line_buf.assert_data_len = 0;
	cmd_line_buf.log_level = 0;
	cmd_line_buf.echo_enable = bTRUE;

	cmd_dev->dev_drv->init(cmd_dev);
	cmd_dev->dev_drv->open(cmd_dev, shell_tx_complete, shell_rx_indicate);

	if(log_dev != cmd_dev)
	{
		log_dev->dev_drv->init(log_dev);
		log_dev->dev_drv->open(log_dev, shell_tx_complete, NULL);
	}

	#ifdef CONFIG_MASTER_CORE
	ipc_dev->dev_drv->init(ipc_dev);
	ipc_dev->dev_drv->open(ipc_dev, NULL);   /* may register rx-callback to copy log data to buffer. */
	#endif

	//os_ext_init();
	create_shell_event();

	shell_init_ok = bTRUE;

}

void shell_task( void *para )
{
	u32    Events;

	shell_task_init();

	echo_out((u8 *)"\r\n#", 3);

	while(bTRUE)
	{
		Events = wait_any_event(BEKEN_WAIT_FOREVER); // WAIT_EVENT;

		if(Events & SHELL_EVENT_TX_REQ)
		{
			echo_out((u8 *)"Unsolicited\r\n#", 14);
		}

		if(Events & SHELL_EVENT_RX_IND)
		{
			rx_ind_process();
		}
	}
}

__weak int shell_log_check(u8 level, char *mod_name)
{
	if(level < cmd_line_buf.log_level)
		return 0; //bFALSE;

	return 1;// bTRUE;
}

int shell_log_out_direct(const char *format, va_list ap)
{
	u32         int_mask;
	char       *pbuf;
	u16         data_len, buf_len;
	va_list     arg_list;

	pbuf = (char *)&cmd_line_buf.assert_buff[0];
	buf_len = sizeof(cmd_line_buf.assert_buff);

	int_mask = rtos_disable_int();

	va_copy(arg_list, ap);
	data_len = vsnprintf( pbuf, buf_len -1, format, arg_list );
	va_end( arg_list );

	if ( (data_len != 0) && (pbuf[data_len - 1] == '\n') ) {
		if (data_len == 1 || pbuf[data_len - 2] != '\r') {
			pbuf[data_len] = '\n';
			pbuf[data_len - 1] = '\r';
			data_len++;
		}
	}

	log_dev->dev_drv->write_sync(log_dev, (u8 *)pbuf, data_len);

	rtos_enable_int(int_mask);

	return 1;
}

void shell_log_raw_data(const u8 *data, u16 data_len)
{
	u8   *packet_buf;
	u16   free_blk_tag;

	if (!shell_init_ok)
	{
		return ; // bFALSE;
	}

	if (NULL == data || 0 == data_len)
	{
		return ; // bFALSE;
	}

	packet_buf = alloc_log_blk(data_len, &free_blk_tag);

	if (NULL == packet_buf)
	{
		return ; // bFALSE;
	}

	memcpy(packet_buf, data, data_len);

	u32 int_mask = rtos_disable_int();

	// push to pending queue.
	push_pending_queue(free_blk_tag, data_len);

	// notify shell task to process the log tx.
	tx_req_process();

	rtos_enable_int(int_mask);

	return ; // bTRUE;
}

void shell_log_out_port(int level, char *mod_name, const char *format, va_list ap)
{
	u8   * packet_buf;
	u16    free_blk_tag;
	u16    log_len = 0, buf_len;
	va_list  arg_list;

	if( !shell_init_ok
		|| rtos_is_in_interrupt_context()
		|| (!rtos_is_scheduler_started()))
	{
		shell_log_out_direct(format, ap);
		return ; // bFALSE;
	}

	if(shell_log_check(level, mod_name) == 0)//bFALSE)
	{
		return ; // bFALSE;
	}

	va_copy(arg_list, ap);
	buf_len = vsnprintf( NULL, 0, format, arg_list ) + 1;
	va_end( arg_list );

	buf_len += MOD_NAME_LEN + 2;

	packet_buf = alloc_log_blk(buf_len, &free_blk_tag);

	if(packet_buf == NULL)
	{
		return ; // bFALSE;
	}

	log_len = 0;

	if(mod_name != NULL)
	{
		memcpy(&packet_buf[0], mod_name, MOD_NAME_LEN);
		packet_buf[MOD_NAME_LEN] = 0;
		log_len = strlen((char *)packet_buf);  // log_len <= MOD_NAME_LEN;
		packet_buf[log_len] = ':';
		log_len++;
		packet_buf[log_len] = 0;
	}

	log_len += vsnprintf( (char *)&packet_buf[log_len], buf_len - log_len, format, ap );
	if ( (log_len != 0) && (packet_buf[log_len - 1] == '\n') ) {
		if (log_len == 1 || packet_buf[log_len - 2] != '\r') {
			packet_buf[log_len] = '\n';
			packet_buf[log_len - 1] = '\r';
			log_len++;
		}
	}

	u32  int_mask = rtos_disable_int();

	// push to pending queue.
	push_pending_queue(free_blk_tag, log_len);

	//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
	tx_req_process();

	rtos_enable_int(int_mask);

	return ; // bTRUE;;
}

int shell_log_out(u8 level, char *mod_name, const char *format, ...)
{
	va_list  arg_list;

	if ( !shell_init_ok )
		return 0; // bFALSE;

	if (shell_log_check(level, mod_name) == 0)
	{
		return 0; // bFALSE;
	}

	va_start(arg_list, format);
	shell_log_out_port(level, mod_name, format, arg_list);
	va_end(arg_list);

	return 1; // bTRUE;;
}

int shell_trace_out( u32 trace_id, ... )
{
	u8    * packet_buf;
	u16     free_blk_tag;
	u16     trace_len = 0, buf_len;

	va_list arg_list;
	u32     arg_num, i, temp;

	if( !shell_init_ok )
		return 0; //bFALSE;

	/* init variable length argument list */
	va_start( arg_list, trace_id );
	arg_num = ( u32 ) va_arg( arg_list, u32 );
	/* clean up variable argument pointer */
	va_end( arg_list );

	if(arg_num > MAX_TRACE_ARGS)
		arg_num = MAX_TRACE_ARGS;
	arg_num++;   /* for  trace_id. */

	buf_len = arg_num * sizeof(u32) * 2 + 2; /* +2 for hdr&tail, *2 for the worst case of escape every byte. */

	packet_buf = alloc_log_blk(buf_len, &free_blk_tag);

	if(packet_buf == NULL)
		return 0; // bFALSE;

	packet_buf[0] = HEX_SYNC_CHAR;
	trace_len = 1;
	trace_len += append_trace_data(&packet_buf[trace_len], buf_len - trace_len, trace_id);

	/* init variable length argument list */
	va_start( arg_list, trace_id );
	arg_num = ( u32 ) va_arg( arg_list, u32 );

	for (i = 0; (i < arg_num) && (i < MAX_TRACE_ARGS); i++)
	{
		temp = ( u32 ) va_arg( arg_list, u32 );

		trace_len += append_trace_data(&packet_buf[trace_len], buf_len - trace_len, temp);
	}

	packet_buf[trace_len] = HEX_SYNC_CHAR;
	trace_len++;

	/* clean up variable argument pointer */
	va_end( arg_list );

	u32  int_mask = rtos_disable_int();

	// push to pending queue.
	push_pending_queue(free_blk_tag, trace_len);

	//set_shell_event(SHELL_EVENT_TX_REQ);  // notify shell task to process the log tx.
	tx_req_process();

	rtos_enable_int(int_mask);

	return 1;//bTRUE;;
}


int shell_assert_out(bool bContinue, char * format, ...)
{
	u32         int_mask;
	char       *pbuf;
	u16         data_len, buf_len;
	va_list     arg_list;

	pbuf = (char *)&cmd_line_buf.assert_buff[0];
	buf_len = sizeof(cmd_line_buf.assert_buff);

	int_mask = rtos_disable_int();

	va_start( arg_list, format );

	data_len = vsnprintf( pbuf, buf_len - 1, format, arg_list );

	va_end( arg_list );

	log_dev->dev_drv->write_sync(log_dev, (u8 *)pbuf, data_len);

	if( bContinue )
	{
		rtos_enable_int(int_mask);
	}
	else
	{
		while(bTRUE) 
		{
		}
	}
	
	return 1;//bTRUE;;

}

#ifdef CONFIG_MASTER_CORE
int shell_log_forward(u8 level, char * log_buf, u16 log_len)
{
	char * mod_name = "CPU1";

	if( !shell_init_ok )
		return 0;//bFALSE;

	if(shell_log_check(level, mod_name) == 0)//bFALSE)
	{
		return 0;//bFALSE;
	}

	/* save log_buf info. */
	shell_log_raw_data((const u8 * )log_buf, log_len);

	return 1;//bTRUE;
}

int shell_cmd_forward(char *cmd, u16 cmd_len)
{
	ipc_dev->dev_drv->write_sync(ipc_dev, (u8 *)cmd, cmd_len);

	return 1;//bTRUE;
}
#endif

void shell_echo_set(int en_flag)
{
	if(en_flag != 0)
		cmd_line_buf.echo_enable = bTRUE;
	else
		cmd_line_buf.echo_enable = bFALSE;
}

int shell_echo_get(void)
{
	if(cmd_line_buf.echo_enable)
		return 1;

	return 0;
}

void shell_set_log_level(int level)
{
	cmd_line_buf.log_level = level;
}

int shell_get_log_level(void)
{
	return cmd_line_buf.log_level;
}


