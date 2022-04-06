#include <common/bk_include.h>
#include "ump3.h"
#include "bk_ump3.h"
#include "bk_usb.h"
#include "bk_uart.h"
#include <os/os.h>
#include "bk_drv_model.h"

#if CONFIG_USB_HOST
#include "diskio.h"
#endif

#define UMP3_STACK_SIZE                     (4 * 1024)
beken_thread_t ump3_thread_handle = NULL;
beken_semaphore_t ump3_device_connect_sema = NULL;

#if CONFIG_USB_HOST
extern void test_mount(DISK_NUMBER number);
extern void test_fatfs(DISK_NUMBER number);
#endif

void um_init(void)
{
	UINT32 ret;
	UINT32 status;
	UINT32 op_flag;
	void *parameter;
	DD_HANDLE usb_handler;

	op_flag = USB_HOST_MODE;
	usb_handler = ddev_open(DD_DEV_TYPE_USB, &status, op_flag);
	if (DD_HANDLE_UNVALID == usb_handler) {
		UM_PRT("usb_open_failed\r\n");
		return;
	}

	parameter = (void *)um_connect_cb;
	ret = ddev_control(usb_handler, UCMD_USB_CONNECTED_REGISTER_CB, parameter);
	if (ret)
		UM_PRT("UCMD_USB_CONNECTED_REGISTER_CB failed\r\n");

	um_work_init();
}

void um_uninit(void)
{
	bk_err_t ret;

	if (ump3_thread_handle) {
		ret = rtos_delete_thread(&ump3_thread_handle);
		ASSERT(kNoErr == ret);
	}

	if (ump3_device_connect_sema)
		rtos_deinit_semaphore(&ump3_device_connect_sema);
}

void um_connect_cb(void)
{
	if (ump3_device_connect_sema) {
		UM_PRT("um_connect_cb\r\n");
		rtos_set_semaphore(&ump3_device_connect_sema);
	}
}

void um_thread_main(void *arg)
{
	bk_err_t result;

	while (1) {
		result = rtos_get_semaphore(&ump3_device_connect_sema, BEKEN_WAIT_FOREVER);
		if (kNoErr == result) {
			UM_PRT("test_mount\r\n");
#if CONFIG_USB_HOST
			test_mount(1);

			UM_PRT("test_fatfs\r\n");
			test_fatfs(1);
#endif
		}
	}
}

uint32_t um_work_init(void)
{
	bk_err_t ret = 0;

	if (ump3_thread_handle)
		goto work_exit;

	ret = rtos_create_thread(&ump3_thread_handle,
							 CONFIG_TASK_UMP3_PRIO,
							 "ump3_thread",
							 (beken_thread_function_t)um_thread_main,
							 (unsigned short)UMP3_STACK_SIZE,
							 (beken_thread_arg_t)0);
	if (kNoErr != ret) {
		UM_PRT("create ump3 thread failed\r\n");
		goto work_exit;
	}

	UM_PRT("um_work_init\r\n");
	if (NULL == ump3_device_connect_sema) {
		ret = rtos_init_semaphore(&ump3_device_connect_sema, 1);
		if (kNoErr != ret) {
			UM_PRT("create device connect sema failed\r\n");
			goto work_exit;
		}
	}

work_exit:
	return (uint32_t)ret;
}
// eof

