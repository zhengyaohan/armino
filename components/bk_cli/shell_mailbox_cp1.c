
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "shell_drv.h"
#include "mailbox_channel.h"

#define TX_QUEUE_LEN     8
#define RX_BUFF_SIZE     160

typedef struct
{
	u8    * packet;
	u16     len;
	u16     tag;
} dev_tx_packet_t;

typedef struct
{
	u8     chnl_id;

	/* ========  TX channel   ======= */
	/* tx queue */
	dev_tx_packet_t    tx_list[TX_QUEUE_LEN];
	u16      list_out_idx;
	u16      list_in_idx;

	/* currently tx packet info */
	u8    *cur_packet;
	u16    packet_len;
	u16    packet_tag;

	u8     tx_stopped;

	tx_complete_t   tx_complete_callback;

	/* ========  RX channel   ======= */
	/* rx buffer */
	u8     rx_buff[RX_BUFF_SIZE + 2];		/* 2 bytes additional for '\r\n' */
	u8     rx_buff_wr_idx;
	u8     rx_buff_rd_idx;
	u16    rx_data_len;

	rx_indicate_t	rx_indicate_callback;

} shell_mb_ext_t;

static bool_t     shell_mb_init(shell_dev_t * shell_dev);
static bool_t     shell_mb_open(shell_dev_t * shell_dev, tx_complete_t tx_callback, rx_indicate_t rx_callback);
static u16        shell_mb_write_async(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen, u16 Tag);
static u16        shell_mb_read(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen);
static u16        shell_mb_write_sync(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen);
static u16        shell_mb_write_echo(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen);
static bool_t     shell_mb_ctrl(shell_dev_t * shell_dev, u8 cmd, void *param);
static bool_t     shell_mb_close(shell_dev_t * shell_dev);

static const shell_dev_drv_t shell_mb_drv = 
	{
		.init    = shell_mb_init,
		.open    = shell_mb_open,
		.write_async  = shell_mb_write_async,
		.read    = shell_mb_read,
		.write_sync = shell_mb_write_sync,
		.write_echo = shell_mb_write_echo,
		.io_ctrl = shell_mb_ctrl,
		.close   = shell_mb_close
	};

static shell_mb_ext_t dev_mb_ext = 
	{
		.chnl_id = MB_CHNL_LOG
	};

shell_dev_t     shell_dev_mb = 
	{
		.dev_drv = (struct _shell_dev_drv *)&shell_mb_drv,
		.dev_type = 0,
		.dev_ext = &dev_mb_ext
	};

/* ===============================  internal functions  =========================== */
static void shell_mb_rx_isr(u8 chnl_id, mb_chnl_cmd_t *cmd_buf)
{
	u32		result = ACK_STATE_FAIL;

	(void)chnl_id;
	shell_mb_ext_t *mb_ext = &dev_mb_ext;

	if(cmd_buf->hdr.cmd == MB_CMD_LOG_OUT_OK)	/* rsp of log_out_cmd, so free the log_buffer. */
	{
		log_cmd_t * log_cmd = (log_cmd_t *)cmd_buf;

		/* do nothing except notifying app to free buffer. */
		if(mb_ext->tx_complete_callback != NULL)
		{
			mb_ext->tx_complete_callback(log_cmd->buf, log_cmd->tag);
		}

		result = ACK_STATE_COMPLETE;
	}
	else if(cmd_buf->hdr.cmd == MB_CMD_USER_INPUT)   /* cmd line inputs. */
	{
		user_cmd_t * user_cmd = (user_cmd_t *)cmd_buf;

		if((user_cmd->len >= RX_BUFF_SIZE) || (mb_ext->rx_buff_wr_idx != mb_ext->rx_buff_rd_idx))
		{
			result = ACK_STATE_FAIL;
		}
		else
		{
			memcpy(mb_ext->rx_buff, user_cmd->buf, user_cmd->len);

			result = ACK_STATE_COMPLETE;    /* it means the buffer can be freed. cmd is pending and will rsp later by MB_CMD_LOG_OUT. */

			mb_ext->rx_data_len = user_cmd->len;
			mb_ext->rx_buff_wr_idx++;	/* prevent mb_rx_isr from receiving new cmd. */
		}

		if(mb_ext->rx_indicate_callback != NULL)
		{
			mb_ext->rx_indicate_callback();
		}
	}
	else   /* unknown cmd. */
	{
		result = ACK_STATE_FAIL;
	}

	/* overwrite the cmd_buf->param3 after the ISR handle complete.
	 * return the ack info to caller using the SAME buffer with cmd buffer.
	 *     !!!! [input as param / outpu as result ]  !!!!
	 */
	mb_chnl_ack_t * ack_buf = (mb_chnl_ack_t *)cmd_buf;
	ack_buf->ack_state = result;

	return;
}

static void shell_mb_tx_isr2(u8 chnl_id)
{
	(void)chnl_id;
	shell_mb_ext_t *mb_ext = &dev_mb_ext;

	mb_chnl_cmd_t	mb_cmd_buf;

	/* next tx. */
	if(mb_ext->list_out_idx != mb_ext->list_in_idx)
	{
		mb_ext->cur_packet = mb_ext->tx_list[mb_ext->list_out_idx].packet;
		mb_ext->packet_len = mb_ext->tx_list[mb_ext->list_out_idx].len;
		mb_ext->packet_tag = mb_ext->tx_list[mb_ext->list_out_idx].tag;

		log_cmd_t * cmd_buf = (log_cmd_t *)&mb_cmd_buf;

		cmd_buf->hdr.data = MB_CMD_LOG_OUT;
		cmd_buf->buf = mb_ext->cur_packet;
		cmd_buf->len = mb_ext->packet_len;
		cmd_buf->tag = mb_ext->packet_tag;

		bk_err_t		ret_code;

		ret_code = mb_chnl_write(mb_ext->chnl_id, &mb_cmd_buf);

		if(ret_code == BK_OK)
		{
			/* to next packet. */
			mb_ext->list_out_idx = (mb_ext->list_out_idx + 1) % TX_QUEUE_LEN;
		}
	}
	else
	{
		/* all packets tx complete. */

		mb_ext->cur_packet = NULL;
		mb_ext->packet_len = 0;

		mb_ext->tx_stopped = 1; /* bTRUE;*/  /* all data tranferred, tx stopped.*/
	}
}

static void shell_mb_tx_cmpl_isr2(u8 chnl_id, mb_chnl_ack_t *ack_buf)
{
	(void)chnl_id;
	shell_mb_ext_t *mb_ext = &dev_mb_ext;

	if(ack_buf->hdr.cmd != MB_CMD_LOG_OUT)
	{
		/*
		 *   !!!  FAULT  !!!
		 */

	//	return;
	}

	/* MB_CMD_LOG_OUT tx complete. */

	if( (ack_buf->hdr.com_fail == 1) 
		|| (ack_buf->ack_state != ACK_STATE_PENDING) )
	{
		/* MB_CMD_LOG_OUT handle complete. */
		/* so notify app to free buffer. */
		if(mb_ext->tx_complete_callback != NULL)
		{
			mb_ext->tx_complete_callback(mb_ext->cur_packet, mb_ext->packet_tag);
		}
	}
}

static void shell_mb_tx_cmpl_isr(u8 chnl_id, mb_chnl_ack_t *ack_buf)
{
	shell_mb_tx_cmpl_isr2(chnl_id, ack_buf);  /*  not the SAME with following code if cmd is NOT MB_CMD_LOG_OUT. */
	shell_mb_tx_isr2(chnl_id);
	return;

	/*      shell_mb_tx_cmpl_isr2     */

	#if 0
	(void)chnl_id;
	shell_mb_ext_t *mb_ext = &dev_mb_ext;

	if(ack_buf->hdr.cmd != MB_CMD_LOG_OUT)
	{
		/*
		 *   !!!  FAULT  !!!
		 */

	//	return;
	}

	/* MB_CMD_LOG_OUT tx complete. */

	if(ack_buf->ack_state != ACK_STATE_PENDING)
	{
		/* MB_CMD_LOG_OUT handle complete. */
		/* so notify app to free buffer. */
		if(mb_ext->tx_complete_callback != NULL)
		{
			mb_ext->tx_complete_callback(mb_ext->cur_packet, mb_ext->packet_tag);
		}
	}
	#endif

	/*      shell_mb_tx_isr2     */

	#if 0
	/* next tx. */
	if(mb_ext->list_out_idx != mb_ext->list_in_idx)
	{
		mb_ext->cur_packet = mb_ext->tx_list[mb_ext->list_out_idx].packet;
		mb_ext->packet_len = mb_ext->tx_list[mb_ext->list_out_idx].len;
		mb_ext->packet_tag = mb_ext->tx_list[mb_ext->list_out_idx].tag;

		log_cmd_t * cmd_buf = (log_cmd_t *)ack_buf;

		cmd_buf->hdr.data = MB_CMD_LOG_OUT;
		cmd_buf->buf = mb_ext->cur_packet;
		cmd_buf->len = mb_ext->packet_len;
		cmd_buf->tag = mb_ext->packet_tag;

		bk_err_t		ret_code;

		ret_code = mb_chnl_write(mb_ext->chnl_id, (mb_chnl_cmd_t *)cmd_buf);

		if(ret_code == BK_OK)
		{
			/* to next packet. */
			mb_ext->list_out_idx = (mb_ext->list_out_idx + 1) % TX_QUEUE_LEN;
		}
	}
	else
	{
		/* all packets tx complete. */

		mb_ext->cur_packet = NULL;
		mb_ext->packet_len = 0;

		mb_ext->tx_stopped = 1; /* bTRUE;*/  /* all data tranferred, tx stopped.*/
	}
	#endif
}

static void shell_mb_tx_trigger(shell_mb_ext_t *mb_ext)
{
	if(mb_ext->tx_stopped == 0)
		return;

	mb_ext->tx_stopped = 0;   // set tx_stopped to 0 firstly, then enable TX.

	shell_mb_tx_isr2(mb_ext->chnl_id);
}

/* ===============================  shell mailbox driver APIs  =========================== */

static bool_t shell_mb_init(shell_dev_t * shell_dev)
{
	u8    dev_id;
	shell_mb_ext_t *mb_ext;

	if(shell_dev == NULL)
		return bFALSE;

	mb_ext = (shell_mb_ext_t *)shell_dev->dev_ext;
	dev_id = mb_ext->chnl_id;

	memset(mb_ext, 0, sizeof(shell_mb_ext_t));
	mb_ext->tx_stopped = 1;
	mb_ext->chnl_id = dev_id;

	bk_err_t	ret_code = mb_chnl_init();

	if(ret_code != BK_OK)
		return bFALSE;

	return bTRUE;
}

static bool_t shell_mb_open(shell_dev_t * shell_dev, tx_complete_t tx_callback, rx_indicate_t rx_callback)
{
	shell_mb_ext_t *mb_ext;

	if(shell_dev == NULL)
		return bFALSE;

	mb_ext = (shell_mb_ext_t *)shell_dev->dev_ext;

	mb_ext->tx_complete_callback = tx_callback;
	mb_ext->rx_indicate_callback = rx_callback;

	mb_chnl_open(mb_ext->chnl_id);
	
	// call chnl driver to register isr callback;
	mb_chnl_ctrl(mb_ext->chnl_id, MB_CHNL_SET_RX_ISR, (void *)shell_mb_rx_isr);
	#if 0
	mb_chnl_ctrl(mb_ext->chnl_id, MB_CHNL_SET_TX_ISR, (void *)shell_mb_tx_isr2);
	mb_chnl_ctrl(mb_ext->chnl_id, MB_CHNL_SET_TX_CMPL_ISR, (void *)shell_mb_tx_cmpl_isr2);
	#else
	mb_chnl_ctrl(mb_ext->chnl_id, MB_CHNL_SET_TX_CMPL_ISR, (void *)shell_mb_tx_cmpl_isr);
	#endif

	return bTRUE;
}

static u16 shell_mb_write_async(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen, u16 Tag)
{
	u16   free_items;
	shell_mb_ext_t *mb_ext;

	if(shell_dev == NULL)
		return 0;

	mb_ext = (shell_mb_ext_t *)shell_dev->dev_ext;

	if((pBuf == NULL) /*|| (BufLen == 0)*/)
		return 0;

	/* enqueue pBuf even if BufLen is 0, upper layer need tx-complete-callback to free this pBuf. */

	if(mb_ext->list_out_idx > mb_ext->list_in_idx)
		free_items = mb_ext->list_out_idx - mb_ext->list_in_idx;
	else
		free_items = TX_QUEUE_LEN - mb_ext->list_in_idx + mb_ext->list_out_idx;

	/* list_out_idx == list_in_idx means empty, so reserved one item. */
	if(free_items > 1)
	{
		mb_ext->tx_list[mb_ext->list_in_idx].packet = pBuf;
		mb_ext->tx_list[mb_ext->list_in_idx].len    = BufLen;
		mb_ext->tx_list[mb_ext->list_in_idx].tag    = Tag;

		mb_ext->list_in_idx = (mb_ext->list_in_idx + 1) % TX_QUEUE_LEN;

		shell_mb_tx_trigger(mb_ext);

		return 1;
	}
	else
	{
		return 0;
	}
}

static u16 shell_mb_read(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen)
{
	u16		read_cnt = 0;

	shell_mb_ext_t *mb_ext;

	if(shell_dev == NULL)
		return 0;

	mb_ext = (shell_mb_ext_t *)shell_dev->dev_ext;

	if(pBuf == NULL)
		return 0;

	if( (mb_ext->rx_buff_rd_idx != mb_ext->rx_buff_wr_idx) && 
		(BufLen >= mb_ext->rx_data_len) )
	{
		read_cnt = mb_ext->rx_data_len;
		memcpy(pBuf, mb_ext->rx_buff, read_cnt);

		mb_ext->rx_buff_rd_idx++;  /* allow mb_rx_isr to recv new cmd. */
	}

	return read_cnt;

}

/* call this after interrupt is DISABLED. */
static u16 shell_mb_write_sync(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen)
{
	return 0;
}

static u16 shell_mb_write_echo(shell_dev_t * shell_dev, u8 * pBuf, u16 BufLen)
{
	return 0;
}

static bool_t shell_mb_ctrl(shell_dev_t * shell_dev, u8 cmd, void *param)
{
	shell_mb_ext_t *mb_ext;

	if(shell_dev == NULL)
		return bFALSE;

	mb_ext = (shell_mb_ext_t *)shell_dev->dev_ext;

	switch(cmd)
	{
		case SHELL_IO_CTRL_GET_STATUS:
			if(param == NULL)
				return bFALSE;
			
			u16   free_items;

			if(mb_ext->list_out_idx > mb_ext->list_in_idx)
				free_items = mb_ext->list_out_idx - mb_ext->list_in_idx;
			else
				free_items = TX_QUEUE_LEN - mb_ext->list_in_idx + mb_ext->list_out_idx;

			if(free_items > 1)
				*((u16 *)param) = free_items - 1;
			else
				*((u16 *)param) = 0;
			
			break;

		case SHELL_IO_CTRL_RX_RESET:
			mb_ext->rx_buff_rd_idx = 0;
			mb_ext->rx_buff_wr_idx = 0;
			mb_ext->rx_data_len    = 0;
			break;

		case SHELL_IO_CTRL_TX_RESET:
			mb_ext->list_out_idx = 0;
			mb_ext->list_in_idx  = 0;

			mb_ext->cur_packet = NULL;
			mb_ext->packet_len = 0;

			break;

		default:
			return bFALSE;
			break;
	}

	return bTRUE;
}

static bool_t shell_mb_close(shell_dev_t * shell_dev)
{
	shell_mb_ext_t *mb_ext;

	if(shell_dev == NULL)
		return bFALSE;

	mb_ext = (shell_mb_ext_t *)shell_dev->dev_ext;

	// call chnl driver to register isr callback;
	mb_chnl_ctrl(mb_ext->chnl_id, MB_CHNL_SET_RX_ISR, NULL);
	mb_chnl_ctrl(mb_ext->chnl_id, MB_CHNL_SET_TX_ISR, NULL);
	mb_chnl_ctrl(mb_ext->chnl_id, MB_CHNL_SET_TX_CMPL_ISR, NULL);

	mb_chnl_close(mb_ext->chnl_id);

	mb_ext->tx_complete_callback = NULL;
	mb_ext->rx_indicate_callback = NULL;

	return bTRUE;
}


