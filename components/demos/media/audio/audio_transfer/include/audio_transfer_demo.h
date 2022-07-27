#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//#include "audio_types.h"
#include "audio_transfer_types.h"
#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#include "ff.h"
#include "diskio.h"
#endif

/* used in cpu0 */
typedef enum {
	AUD_TRAS_IDLE = 0,
	AUD_TRAS_TX_MIC_DATA,
	AUD_TRAS_READ_DONE,
	AUD_TRAS_WRITE_REQ,
	AUD_TRAS_WRITE_DONE,
	AUD_TRAS_READY,
	AUD_TRAS_EXIT,
	AUD_TRAS_MAX,
} audio_tras_opcode_t;

typedef struct {
	audio_tras_opcode_t op;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
} audio_tras_msg_t;


typedef struct {
	bool aec_enable;
	audio_tras_samp_rate_t samp_rate;
	int (*audio_send_mic_data)(unsigned char *data, unsigned int len);
} audio_tras_demo_setup_t;

typedef enum {
	AUD_TRAS_STA_IDLE = 0,
	AUD_TRAS_STA_INITING,
	AUD_TRAS_STA_READY,
	AUD_TRAS_STA_WORKING,
	AUD_TRAS_STA_STOPING,
	AUD_TRAS_STA_MAX,
} audio_tras_status_t;

typedef struct {
	audio_tras_status_t status;
#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
	char sin_file_name[50];
	char ref_file_name[50];
	char out_file_name[50];
	FIL sin_file;
	FIL ref_file;
	FIL out_file;
#endif
} audio_tras_demo_info_t;

bk_err_t audio_tras_demo_init(audio_tras_demo_setup_t demo_setup);

bk_err_t audio_tras_demo_deinit(void);

bk_err_t audio_tras_demo_write_dac_data(uint8_t *dac_buff, uint32_t len);

#ifdef __cplusplus
}
#endif

