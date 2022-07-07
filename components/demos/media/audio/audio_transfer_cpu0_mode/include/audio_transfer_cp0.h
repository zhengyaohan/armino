#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "audio_transfer_types.h"

bk_err_t bk_audio_transfer_init(audio_setup_t *setup_cfg);

bk_err_t bk_audio_transfer_deinit(void);

void bk_audio_register_rw_cb(int (*send_audio_data)(unsigned char *data, unsigned int len),
									void (*transfer_init_callback)(void));

bk_err_t bk_audio_write_data(uint8_t *data_buf, uint32_t length);

#ifdef __cplusplus
}
#endif

