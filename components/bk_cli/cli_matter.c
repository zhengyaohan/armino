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

#include <stdlib.h>
#include <string.h>
#include "func_convert.h"
#include "cli.h"
#include "flash_namespace_value.h"



extern void BkQueryImageCmdHandler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv );
extern void BkApplyUpdateCmdHandler(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv );
extern void ChipTest(void);


void matter_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{ 
    beken_thread_t matter_thread_handle = NULL;

    os_printf("start matter\r\n");
    rtos_create_thread(&matter_thread_handle,
							 BEKEN_DEFAULT_WORKER_PRIORITY,
							 "matter",
							 (beken_thread_function_t)ChipTest,
							 8192,
							 0);
    
}


void matter_wr(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t *val=NULL;
    uint32_t idx, len, ret;
    if(argc < 5){
        os_printf("argc less\r\n");
        return;
    }
    if((argc > 5) && (argv[2][0] == '1') ){
        len = strlen(argv[5]);
        val = os_malloc((len+1)/2);
        str_to_hexarray(argv[5], val);
    }
    switch(argv[1][0])
    {
        case 'w':
        if(argv[2][0] == '1')
            bk_write_data(argv[3], argv[4], (char *)val, (len+1)/2);
        else
            bk_write_data(argv[3], argv[4], argv[5], strlen(argv[5]) );
        break;
        case 'r':
        val = os_malloc(600);
        ret = bk_read_data(argv[3], argv[4], (char *)val, 600, &len);
        if(ret == kNoErr){
            os_printf("data = ");
            for(idx=0; idx<len; idx++){
                os_printf("%02x ", val[idx]);
            }
            os_printf("\r\n");
        }
        else
            os_printf("key not exist \r\n");
    }
    if(val != NULL)
        os_free(val);
}


#define NAMEKEY_CMD_CNT              (sizeof(s_nameKey_commands) / sizeof(struct cli_command))
static const struct cli_command s_nameKey_commands[] =
{
    {
        "bk_write_data_test", "bk_write_data_test", bk_write_data_test
    },
    {
        "bk_read_data_test", "bk_read_data_test", bk_read_data_test
    },
    {
        "bk_erase_namespace_test", "bk_erase_namespace_test", bk_erase_namespace_test
    },
    {
        "bk_erase_name_test", "bk_erase_name_test", bk_erase_name_test
    },
    {
        "bk_name_data_exits", "bk_name_data_exits", bk_name_data_exits
    },
    {
        "bk_erase_all_test", "bk_erase_all_test", bk_erase_all_test
    },
    {
        "matter", "matter", matter_test
    },
    {
        "BkQueryImageCmdHandler", "BkQueryImageCmdHandler", BkQueryImageCmdHandler
    },
    {
        "BkApplyUpdateCmdHandler", "BkApplyUpdateCmdHandler", BkApplyUpdateCmdHandler
    },
    {
        "matter_wr", "matter_wr op(w/r) type(0/1) ns key value", matter_wr
    },
};


int cli_matter_init(void)
{
    return cli_register_commands(s_nameKey_commands, NAMEKEY_CMD_CNT);
}


