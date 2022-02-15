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

#include <stdio.h>
#include <stdlib.h>
#include "bk_include.h"
#include "bk_private/components_init.h"
#include "rtos_init.h"
#include "bk_api_rtos.h"
#include "sys_driver.h"

//#define RTOS_FUNC_TEST
#ifdef RTOS_FUNC_TEST
/*rtos thread func test, for bk7256 bringup.*/
#include "FreeRTOS.h"
#include "queue.h"

beken_queue_t test_thread_msg_que = NULL;
beken_semaphore_t test_thread_sema = NULL;


beken_thread_t test_sned_thread_handle = NULL;
beken_thread_t test_rec_thread_handle = NULL;

extern void vTaskDelay();
static void test_send_thread_func(void* parameter)
{
	int msg = 1;
	int ret = 0;

	while(1)
	{
		//write msg to queue
		ret = rtos_push_to_queue(&test_thread_msg_que, &msg, 0xffffffff );
		if(ret < 0){
			os_printf("test thread send msg failed\r\n");
			while(1);
		}
		os_printf("test_send_thread_func send msg %d\r\n", msg);

		vTaskDelay(1000);
		msg++;
		//give binary semaphore
		if(msg <10){
			ret = rtos_set_semaphore(&test_thread_sema);
			if(ret < 0){
				os_printf("get sema failed");
			}
		}
	}
}

static void test_rec_thread_func(void* parameter)
{
	int msg = 0;
	int ret = 0;

	while(1)
	{
		//get semaphore
		ret = rtos_get_semaphore(&test_thread_sema, BEKEN_WAIT_FOREVER);
		if(ret < 0){
			os_printf("get sema failed");
		}

		//read msg from queue
		ret = rtos_pop_from_queue(&test_thread_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if(ret < 0){
			os_printf("get smg failed");
		}
		os_printf("test_rec_thread_func rec msg %d\r\n", msg);
	}
}

void rtos_thread_func_test()
{
	int ret = 0;
	//init queue,depth 4, size 4 Bytes
	ret = rtos_init_queue(&test_thread_msg_que,
							  "thread_test_queue",
							  4,
							  4);
	if(ret < 0){
		os_printf("init queue failed");
	}

	//init binary semaphore
	ret = rtos_init_semaphore(&test_thread_sema, 1);
	if(ret < 0){
		os_printf("init sema failed");
	}

	// create send thread
	ret = rtos_create_thread(test_sned_thread_handle, 4,
		"test_send_thread",
		(beken_thread_function_t)test_send_thread_func,
		512,
		(beken_thread_arg_t)0);
	if(ret < 0){
		os_printf("error:@thread send creat failed @");
	}

	// create rec thread
	ret =rtos_create_thread(test_rec_thread_handle, 4,
		"test_rec_thread",
		(beken_thread_function_t)test_rec_thread_func,
		512,
		(beken_thread_arg_t)0);

	if(ret < 0){
		os_printf("error:@thread rec creat failed @");
	}
	os_printf("success :@thread rec creat\r\n @");
	//if necessary, close the main().
}
#endif

#if (CONFIG_MASTER_CORE) 
void reset_slave_core(uint32 offset, uint32_t reset_value)
{
	if (0 != reset_value  && 1 != reset_value) {
		os_printf("reset_value must be 0 or 1.\r\n");
		reset_value = CONFIG_SLAVE_CORE_RESET_VALUE;
	} 
	os_printf("reset_slave_core at: %08x, reset value:%d\r\n", offset, reset_value);
	sys_drv_set_cpu1_boot_address_offset(offset >> 8);
	sys_drv_set_cpu1_reset(reset_value);
}

void start_slave_core(void)
{
#if (CONFIG_SLAVE_CORE_OFFSET && CONFIG_SLAVE_CORE_RESET_VALUE) 
	reset_slave_core(CONFIG_SLAVE_CORE_OFFSET, CONFIG_SLAVE_CORE_RESET_VALUE);
#endif
	os_printf("start_slave_core end.\r\n");
}

void stop_slave_core(void)
{
#if (CONFIG_SLAVE_CORE_OFFSET && CONFIG_SLAVE_CORE_RESET_VALUE) 
	uint32_t reset_value = ( 0x1) & (~CONFIG_SLAVE_CORE_RESET_VALUE);
	reset_slave_core(CONFIG_SLAVE_CORE_OFFSET, reset_value);
#endif
	os_printf("start_slave_core end.\r\n");
}

#endif

extern int main(void);
static void app_main_thread(void *arg)
{
#ifdef RTOS_FUNC_TEST
	/*rtos thread func test, for bk7256 bringup.*/
	rtos_thread_func_test();
	//if nessary ,close the main() function.
#endif
	main();

#if (CONFIG_MASTER_CORE) 
	start_slave_core();
#endif

	rtos_delete_thread(NULL);
}

void start_app_main_thread(void)
{
	rtos_create_thread(NULL, CONFIG_APP_MAIN_TASK_PRIO,
		"main",
		(beken_thread_function_t)app_main_thread,
		CONFIG_APP_MAIN_TASK_STACK_SIZE,
		(beken_thread_arg_t)0);
}

void __attribute__((weak)) OHOS_SystemInit(void)
{
	os_printf("@NOOS@\r\n");
    return;
}

void entry_main(void)
{
	rtos_init();
	components_init();

	start_app_main_thread();
	OHOS_SystemInit();

	rtos_start_scheduler();

}

