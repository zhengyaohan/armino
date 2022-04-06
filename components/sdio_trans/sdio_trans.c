#include <common/bk_include.h>
#include "bk_arm_arch.h"
#include "bk_sdio.h"
#include "sdio_trans.h"
#include "bk_sdio_intf.h"
#include "bk_sdio.h"
#include "bk_drv_model.h"
#include <os/mem.h>
//#include "mac.h"
//#include "ke_event.h"
//#include "llc.h"
//#include "txu_cntrl.h"
//#include "txl_cntrl.h"
#include <os/mem.h>
//#include "co_utils.h"
//#include "scan_task.h"
//#include "scanu_task.h"
//#include "me_task.h"
//#include "mm_task.h"
//#include "sm_task.h"
//#include "vif_mgmt.h"
//#include "apm_task.h"
//#include "rxu_task.h"
#include "bk_uart.h"
#include <os/mem.h>
//#include "apm_task.h"
//#include "mac_ie.h"

//#include "hostapd_cfg.h"

#if CONFIG_TEMP_DETECT
#include "temp_detect_pub.h"
#endif

//#include "common.h"
//#include "rwnx.h"
//#include "bk_rw.h"

#if  CONFIG_SDIO_TRANS

static DD_HANDLE sdio_hdl;

SDIO_NODE_PTR sdio_trans_get_node(void)
{
	UINT32 status;
	UINT32 rd_sta;
	DD_HANDLE sdio_hdl;
	SDIO_NODE_PTR ret = 0;
	SDIO_NODE_PTR mem_node_ptr;

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl)
		goto rxed_exception;

	rd_sta = ddev_read(sdio_hdl, (char *)SDIO_DUMMY_BUFF_ADDR, SDIO_DUMMY_LENGTH, H2S_RD_SPECIAL);
	if (SDIO_FAILURE == rd_sta)
		goto rd_fail;
	else {
		mem_node_ptr = (SDIO_NODE_PTR)rd_sta;
		ret = mem_node_ptr;
	}

rd_fail:

rxed_exception:
	return ret;
}

UINT32 sdio_trans_release_node(SDIO_NODE_PTR mem_node_ptr)
{
	UINT32 status;
	DD_HANDLE sdio_hdl;
	UINT32 ret = SDIO_INTF_SUCCESS;

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl) {
		ret = SDIO_INTF_FAILURE;

		goto rxed_exception;
	}

	ddev_control(sdio_hdl, SDIO_CMD_PUSH_FREE_NODE, (void *)mem_node_ptr);

rxed_exception:
	return ret;
}
void sdio_trans_evt(int dummy)
{
	SDIO_NODE_PTR mem_node_ptr;
	STM32_FRAME_HDR *frm_hdr_ptr;

	mem_node_ptr = sdio_trans_get_node();

	while (mem_node_ptr) {
		frm_hdr_ptr = (STM32_FRAME_HDR *)mem_node_ptr->addr;

		app_lwip_udp_send_packet(frm_hdr_ptr, mem_node_ptr->length);
		os_printf("sdio recv len:%d\r\n", mem_node_ptr->length);

		sdio_trans_release_node(mem_node_ptr);

		mem_node_ptr = sdio_trans_get_node();
	}

	if (0 == mem_node_ptr)
		ke_evt_clear(KE_EVT_SDIO_TRANS_DATA_BIT);

	return;
}

void sdio_trans_trigger_evt(void)
{
	ke_evt_set(KE_EVT_SDIO_TRANS_DATA_BIT);
}
void sdio_trans_close(void)
{
	ddev_close(sdio_hdl);
}
int sdio_trans_init(void)
{
	UINT32 ret;
	UINT32 status;

	ret = SDIO_TRANS_FAILURE;

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl)
		goto init_exception;

	ddev_control(sdio_hdl, SDIO_CMD_REG_RX_CALLBACK, (void *)sdio_trans_trigger_evt);

	SDIO_TRANS_PRT("sdio_intf_init\r\n");
	ret = SDIO_TRANS_SUCCESS;

init_exception:
	return ret;
}

#endif

