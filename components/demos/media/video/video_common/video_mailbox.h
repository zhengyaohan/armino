#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "mailbox_channel.h"


typedef struct {
	uint32_t chnl_id;
} video_mb_t;

typedef enum {
	VID_MB_FRAME_BUFF_READY_CMD = 0,
	VID_MB_FRAME_BUFF_SET_STATE_CMD,
	VID_MB_FRAME_BUFF_REQUEST_CMD,
	VID_MB_FRAME_BUFF_RESPONSE_CMD,
	VID_MB_CPU0_EXIT_CMD,
	VID_MB_CPU1_EXIT_CMD,
	VID_MB_MAX_CMD,
} video_maibox_cmd_t;

typedef struct {
	video_maibox_cmd_t mb_cmd;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
} video_mailbox_msg_t;

void video_mailbox_init(void *rx_isr, void *tx_isr, void *tx_cmpl_isr);
void video_mailbox_deinit(void);
bk_err_t video_mailbox_send_msg(video_mailbox_msg_t *msg);

#ifdef __cplusplus
}
#endif

