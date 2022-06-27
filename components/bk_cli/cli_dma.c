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

#include <os/os.h>
#include "cli.h"
#include <driver/dma.h>
#include "bk_general_dma.h"

#define SEND_BUF_LEN 1024

static void cli_dma_help(void)
{
    CLI_LOGI("dma_driver {init|deinit}\n");
    CLI_LOGI("dma {id} {init|deinit|start|stop}\n");
    CLI_LOGI("dma_int {id} {reg|enable|disable}\n");
    CLI_LOGI("dma_chnl alloc \n");
    CLI_LOGI("dma_chnl_free {id} \n");
    CLI_LOGI("dma_memcopy_test {copy} {count|in_number1|in_number2|out_number1|out_number2}\r\n");
}

static void cli_dma_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        cli_dma_help();
        return;
    }

    if (os_strcmp(argv[1], "init") == 0) {
        BK_LOG_ON_ERR(bk_dma_driver_init());
        CLI_LOGI("dma driver init\n");
    } else if (os_strcmp(argv[1], "deinit") == 0) {
        BK_LOG_ON_ERR(bk_dma_driver_deinit());
        CLI_LOGI("dma driver deinit\n");
    } else {
        cli_dma_help();
        return;
    }
}

static void cli_dma_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t id;

    if (argc < 2) {
        cli_dma_help();
        return;
    }

    id = os_strtoul(argv[1], NULL, 10);

    if (os_strcmp(argv[2], "init") == 0) {
        CLI_RET_ON_INVALID_ARGC(argc, 3);
        dma_config_t config = {
            .mode = DMA_WORK_MODE_SINGLE,
            .chan_prio = 2,
            .src = {
                .dev = DMA_DEV_DTCM,
                .width = DMA_DATA_WIDTH_32BITS,
                .addr_inc_en = DMA_ADDR_INC_ENABLE,
                .addr_loop_en = DMA_ADDR_LOOP_DISABLE,
                .start_addr = 0,
                .end_addr = 0,
            },
            .dst = {
                .dev = DMA_DEV_UART1,
                .width = DMA_DATA_WIDTH_8BITS,
                .addr_inc_en = DMA_ADDR_INC_DISABLE,
                .addr_loop_en = DMA_ADDR_LOOP_DISABLE,
                .start_addr = 0x0802100 + 0x3 * 4,
                .end_addr = 0,
            },
        };
        BK_LOG_ON_ERR(bk_dma_init(id, &config));
        CLI_LOGI("dma init, id=%d\n", id);
    }else if (os_strcmp(argv[2], "start") == 0) {
        BK_LOG_ON_ERR(bk_dma_start(id));
        CLI_LOGI("dma start, id=%d\n", id);
    } else if (os_strcmp(argv[2], "stop") == 0) {
        BK_LOG_ON_ERR(bk_dma_stop(id));
        CLI_LOGI("dma stop, id=%d\n", id);
    } else if (os_strcmp(argv[2], "deinit") == 0) {
        BK_LOG_ON_ERR(bk_dma_deinit(id));
        CLI_LOGI("dma deinit, id=%d\n", id);
    } else if (os_strcmp(argv[2], "set_tran_len") == 0) {
        BK_LOG_ON_ERR(bk_dma_set_transfer_len(id, SEND_BUF_LEN));
        CLI_LOGI("dma set tran len, id=%d, len=%d\n", id, SEND_BUF_LEN);
    } else if (os_strcmp(argv[2], "get_remain_len") == 0) {
        uint32_t remain_len = bk_dma_get_remain_len(id);
        CLI_LOGI("dma get remain_len, id=%d, len=%x\n", id, remain_len);
    }else {
        cli_dma_help();
        return;
    }
}

static void cli_dma_half_finish_isr(dma_id_t dma_id)
{
    CLI_LOGI("dma half finish isr(%d)\n", dma_id);
}

static void cli_dma_finish_isr(dma_id_t dma_id)
{
    CLI_LOGI("dma finish isr(%d)\n", dma_id);
}

static void cli_dma_int_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t id;

    if (argc != 3) {
        cli_dma_help();
        return;
    }

    id = os_strtoul(argv[1], NULL, 10);

    if (os_strcmp(argv[2], "reg") == 0) {
        BK_LOG_ON_ERR(bk_dma_register_isr(id, cli_dma_half_finish_isr, cli_dma_finish_isr));
        CLI_LOGI("dma id:%d register interrupt isr\n", id);
    } else if (os_strcmp(argv[2], "enable_hf_fini") == 0) {
        BK_LOG_ON_ERR(bk_dma_enable_half_finish_interrupt(id));
        CLI_LOGI("dma id%d enable half finish interrupt\n", id);
    } else if (os_strcmp(argv[2], "disable_hf_fini") == 0) {
        BK_LOG_ON_ERR(bk_dma_disable_half_finish_interrupt(id));
        CLI_LOGI("dma id%d disable half finish interrupt\n", id);
    } else if (os_strcmp(argv[2], "enable_fini") == 0) {
        BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(id));
        CLI_LOGI("dma id%d enable finish interrupt\n", id);
    } else if (os_strcmp(argv[2], "disable_fini") == 0) {
        BK_LOG_ON_ERR(bk_dma_disable_finish_interrupt(id));
        CLI_LOGI("dma id%d disable finish interrupt\n", id);
    } else {
        cli_dma_help();
        return;
    }
}

static void cli_dma_chnl_alloc(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t id;

    if (argc < 2) {
        cli_dma_help();
        return;
    }

    if (os_strcmp(argv[1], "alloc") == 0) {
	 id = bk_dma_alloc(DMA_DEV_MAX);
        CLI_LOGI("dma channel id:%x\n", id);
    } else {
        cli_dma_help();
        return;
    }
}

static void cli_dma_chnl_free(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t id;
    uint32_t ret;

    if (argc < 2) {
        cli_dma_help();
        return;
    }

    if (os_strcmp(argv[1], "free") == 0) {
	id = os_strtoul(argv[2], NULL, 10);
	ret = bk_dma_free(DMA_DEV_MAX, id);
	CLI_LOGI("dma channel free id:%d\n", ret);
    } else {
	CLI_LOGI("cli_dma_chnl_free NOT free\n");
       cli_dma_help();
       return;
    }
}

static void cli_dma_memcpy_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{

    if (argc < 7) {
        cli_dma_help();
        return;
    }
    uint32_t *out_buffer = NULL;
    uint32_t *in_buffer = NULL;
    uint32_t in_write_buffer[2] = {0};
	uint32_t out_write_buffer[2] = {0};

    int count = 1;
    if (os_strcmp(argv[1], "copy") == 0) {
        count = os_strtoul(argv[2], NULL, 10);

        in_write_buffer[0] = os_strtoul(argv[3], NULL, 10);
        CLI_LOGD("cli_dma_memcpy_TEST IN_buffer[0]: %d\r\n", in_write_buffer[0]);
        in_write_buffer[1] = os_strtoul(argv[4], NULL, 10);
        CLI_LOGD("cli_dma_memcpy_TEST IN_buffer[1]: %d\r\n", in_write_buffer[1]);
        in_buffer = in_write_buffer;


        out_write_buffer[0] = os_strtoul(argv[5], NULL, 10);
        CLI_LOGD("cli_dma_memcpy_TEST OUT_buffer[0]: %d\r\n", out_write_buffer[0]);
        out_write_buffer[1] = os_strtoul(argv[6], NULL, 10);
        CLI_LOGD("cli_dma_memcpy_TEST OUT_buffer[1]: %d\r\n", out_write_buffer[1]);
        out_buffer = out_write_buffer;

        while(count) {
              for(int j = 0; j < 2; j++) {
                  CLI_LOGI("cli_dma_memcpy_test BEFORE IN_buffer[%d]: %d\r\n", j,in_buffer[j]);
                  CLI_LOGI("cli_dma_memcpy_test BEFORE OUT_buffer[%d]: %d\r\n", j,out_buffer[j]);
              }
              CLI_LOGI("===============================================\r\n");

              dma_memcpy(out_buffer, in_buffer, 2 * sizeof(uint32_t));

              for(int i = 0; i < 2; i++) {
                  CLI_LOGI("cli_dma_memcpy_test AFTER IN_buffer[%d]: %d\r\n", i,in_buffer[i]);
                  CLI_LOGI("cli_dma_memcpy_test AFTER OUT_buffer[%d]: %d\r\n", i,out_buffer[i]);
              }
              CLI_LOGI("#############################################\r\n");

              count--;
        }

    } else {
        cli_dma_help();
        return;
    }
}

#define DMA_CMD_CNT (sizeof(s_dma_commands) / sizeof(struct cli_command))
static const struct cli_command s_dma_commands[] = {
    {"dma_driver", "dma_driver {init|deinit}", cli_dma_driver_cmd},
    {"dma", "dma {id} {init|deinit|start|stop|set_tran_len|get_remain_len}", cli_dma_cmd},
    {"dma_int", "dma_int {id} {reg|enable_hf_fini|disable_hf_fini|enable_fini|disable_fini}", cli_dma_int_cmd},
    {"dma_chnl", "dma_chnl alloc", cli_dma_chnl_alloc},
    {"dma_chnl_free", "dma_chnl_free {id}", cli_dma_chnl_free},
    {"dma_memcopy_test", "copy {count|in_number1|in_number2|out_number1|out_number2}", cli_dma_memcpy_test},
};

int cli_dma_init(void)
{
    BK_LOG_ON_ERR(bk_dma_driver_init());
    return cli_register_commands(s_dma_commands, DMA_CMD_CNT);
}

