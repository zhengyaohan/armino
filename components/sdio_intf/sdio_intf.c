#include <common/bk_include.h>
#include "bk_arm_arch.h"

//#include "rwnx.h"
//#include "bk_rw.h"
#include "bk_sdio.h"
#include "sdio_intf.h"
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
//#include "apm_task.h"
//#include "rxu_task.h"

#include "bk_uart.h"
#include <os/mem.h>
//#include "vif_mgmt.h"

#include "sdio_common.h"


#if CONFIG_SDIO
static LIST_HEADER_T inbound_list;
static SDIO_NODE_PTR temp_mem_node_ptr;
static INT32 sdio_tx_done(struct tx_cfm_tag *cfm);

static SDIO_NODE_PTR sdio_get_rxed_node(void)
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
	//ddev_close(sdio_hdl);

rxed_exception:
	return ret;
}

static UINT32 sdio_get_free_node_count(void)
{
	UINT32 status;
	UINT32 count = 0;
	DD_HANDLE sdio_hdl;

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl) {
		count = 0;

		goto rxed_exception;
	}

	ddev_control(sdio_hdl, SDIO_CMD_GET_CNT_FREE_NODE, &count);

rxed_exception:
	return count;
}

static UINT32 sdio_get_free_node(void **buf_pptr, UINT32 buf_size, UINT8 tid)
{
	UINT32 status;
	DD_HANDLE sdio_hdl;
	UINT32 ret = SDIO_INTF_SUCCESS;
	struct get_free_node_param param = {buf_size, tid};

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl) {
		ret = SDIO_INTF_FAILURE;

		goto rxed_exception;
	}

	temp_mem_node_ptr = (SDIO_NODE_PTR)ddev_control(sdio_hdl, SDIO_CMD_GET_FREE_NODE, &param);
	if (temp_mem_node_ptr) {
		*buf_pptr = temp_mem_node_ptr->addr;
		temp_mem_node_ptr->length = buf_size;
	} else
		*buf_pptr = 0;

rxed_exception:
	return ret;
}

static UINT32 sdio_release_one_node(SDIO_NODE_PTR mem_node_ptr)
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

UINT32 sdio_emb_get_tx_info(UINT8 *buf, UINT8 *tid)
{
	*tid = 0xFF;

	return SDIO_INTF_SUCCESS;
}

UINT32 sdio_emb_get_hwqueue_id(UINT8 tid)
{
	return AC_VI;
}

void sdio_emb_txdesc_copy(struct txdesc *dst_local, ETHHDR_PTR eth_hdr_ptr)
{
	struct hostdesc *host_ptr;

	host_ptr = &dst_local->host;

	os_memcpy(&host_ptr->eth_dest_addr, &eth_hdr_ptr->h_dest, sizeof(host_ptr->eth_dest_addr));
	os_memcpy(&host_ptr->eth_src_addr, &eth_hdr_ptr->h_src, sizeof(host_ptr->eth_src_addr));
}

UINT32 outbound_upload_data(RW_RXIFO_PTR rx_info)
{
	UINT8 *buf_ptr = rx_info->data;
	//UINT32 len = rx_info->length;

	UINT32 status;
	DD_HANDLE sdio_hdl;
	UINT32 ret = SDIO_INTF_SUCCESS;
	SDIO_NODE_PTR node = temp_mem_node_ptr;
	STM32_FRAME_HDR *frm_hdr_ptr;

	BK_ASSERT(buf_ptr == node->addr);
	BK_ASSERT(node->length);

	frm_hdr_ptr = (STM32_FRAME_HDR *)node->addr;
	frm_hdr_ptr->len = node->length;
	frm_hdr_ptr->type = MVMS_DAT;

	/* filled with receive vectors, don't override them */
	//rx_ptr = (STM32_RXPD_PTR)&frm_hdr_ptr[1];
	//os_memset(rx_ptr, 0, sizeof(STM32_RXPD_S));
	//rx_ptr->pkt_ptr = 0x36; // 0x4e;

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl) {
		ret = SDIO_INTF_FAILURE;

		goto rxed_exception;
	}

	ddev_write(sdio_hdl, (char *)node, SDIO_DUMMY_LENGTH, S2H_WR_SPECIAL);

rxed_exception:

	return ret;
}

void inbound_cfm(void *_cfm)
{
	struct tx_cfm_tag *cfm = (struct tx_cfm_tag *)_cfm;
	//LIST_HEADER_T *tmp;
	//LIST_HEADER_T *pos;
	SDIO_NODE_PTR mem_node_ptr;

	//GLOBAL_INT_DECLARATION();

	//SDIO_INTF_PRT("inbound_cfm\r\n");

	/* notify host */
	if (cfm)
		sdio_tx_done(cfm);

	/* free node */
	mem_node_ptr = (SDIO_NODE_PTR)cfm->addr;
	if (likely(mem_node_ptr)) {
		if (mem_node_ptr->callback)
			mem_node_ptr->callback(mem_node_ptr->Lparam, mem_node_ptr->Rparam);

		mem_node_ptr->callback = NULLPTR;
		mem_node_ptr->Lparam   = NULL;
		mem_node_ptr->Rparam   = NULL;

		sdio_release_one_node(mem_node_ptr);
	}

#if 0
	GLOBAL_INT_DISABLE();

	node = NULLPTR;
	list_for_each_safe(pos, tmp, &inbound_list) {
		list_del(pos);
		node = list_entry(pos, SDIO_NODE_T, node_list);

		if (node->callback)
			node->callback(node->Lparam, node->Rparam);

		node->callback = NULLPTR;
		node->Lparam   = NULL;
		node->Rparam   = NULL;

		sdio_release_one_node(node);

		break;
	}
	GLOBAL_INT_RESTORE();
#endif

	return;
}


UINT32 sdio_resp_conversion(struct ke_msg *msg, STM32_FRAME_HDR **frm_pptr)
{
	UINT16 len;
	STM32_FRAME_HDR *frm;

	len = sizeof(STM32_FRAME_HDR) + msg->param_len + sizeof(*msg);	/* no need precise value */
	frm = (STM32_FRAME_HDR *)os_zalloc(len);
	BK_ASSERT(frm);

	frm->len = len;
	frm->type = MVMS_CMD;
	*frm_pptr = frm;

	/* ** double check here ** */
	os_memcpy(frm + 1, &msg->id, len - sizeof(STM32_FRAME_HDR) - offsetof(struct ke_msg, id));

	//print_hex_dump("KEMSG: ", frm, len);

	return len;
}

UINT32 sdio_emb_kmsg_fwd(struct ke_msg *msg)
{
	UINT32 ret;
	UINT32 len;
	UINT32 status;
	UINT32 wr_sta;
	DD_HANDLE sdio_hdl;
	STM32_FRAME_HDR *frm_ptr;

	ret = SDIO_INTF_FAILURE;

	/*prepare for response content*/
	len = sdio_resp_conversion(msg, &frm_ptr);

	ke_msg_free(msg);

	if (0 == len)
		goto tx_ok;

	/*forward the new message*/
	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl)
		goto tx_exception;

	wr_sta = ddev_write(sdio_hdl, (char *)frm_ptr, len, S2H_WR_SYNC);
	if (SDIO_FAILURE == wr_sta)
		goto tx_exception;

	if (frm_ptr)
		os_free(frm_ptr);

tx_ok:
	ret = SDIO_INTF_SUCCESS;

tx_exception:

	return ret;
}

UINT32 sdio_h2e_msg_conversion(STM32_FRAME_HDR *frm_ptr, struct ke_msg **kmsg_pptr)
{
	UINT32 ret = SDIO_INTF_SUCCESS;
	struct ke_msg *kmsg_dst;

	if (frm_ptr->len <= sizeof(STM32_FRAME_HDR)) {
		ret = SDIO_INTF_FAILURE;
		goto conv_exit;
	}

	kmsg_dst = (struct ke_msg *)os_malloc(frm_ptr->len - sizeof(STM32_FRAME_HDR) + 8/* sizeof(struct co_list_hdr)*/);
	BK_ASSERT(kmsg_dst);
	os_memcpy(&kmsg_dst->id, frm_ptr + 1, frm_ptr->len - sizeof(STM32_FRAME_HDR));
	os_printf("msg: id 0x%x, dstid 0x%x, srcid 0x%x, len %d\n", kmsg_dst->id,
			  kmsg_dst->dest_id, kmsg_dst->src_id, kmsg_dst->param_len);

	/* some address must be tuned */
	switch (kmsg_dst->id) {
	case SCAN_START_REQ: {
		struct scan_start_req *req = ke_msg2param(kmsg_dst);
		req->add_ies = (uint32_t)req->ies;
	}
	break;
	case SCANU_START_REQ: {
		struct scanu_start_req *req = ke_msg2param(kmsg_dst);
		req->add_ies = (uint32_t)req->ies;
	}
	break;
	case APM_START_REQ: {
		struct apm_start_req *req = ke_msg2param(kmsg_dst);
		req->bcn_addr = (uint32_t)req->bcn_buf;
	}
	break;
	case MM_BCN_CHANGE_REQ: {
		struct mm_bcn_change_req *req = ke_msg2param(kmsg_dst);
		req->bcn_ptr_malloc_flag = false;
		req->bcn_ptr = (uint32_t)req->bcn_buf;
	}
	break;
	default:
		break;
	}

	*kmsg_pptr = kmsg_dst;

conv_exit:
	return ret;
}

void sdio_h2e_kmsg_hdlr(struct ke_msg *kmsg_ptr)
{
	ke_msg_send(ke_msg2param(kmsg_ptr));
}

INT32 cmd_ack_host(UINT8 seq)
{
	UINT32 status;
	DD_HANDLE sdio_hdl;
	STM32_FRAME_HDR frm_hdr_ptr;

	frm_hdr_ptr.len = sizeof(STM32_FRAME_HDR);
	frm_hdr_ptr.type = MVMS_LLIND;
	frm_hdr_ptr.seq = seq;

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl)
		return SDIO_INTF_FAILURE;

	ddev_write(sdio_hdl, (char *)&frm_hdr_ptr, sizeof(frm_hdr_ptr), S2H_WR_SYNC);

	return SDIO_INTF_SUCCESS;
}

static INT32 sdio_tx_done(struct tx_cfm_tag *cfm)
{
#ifdef CONFIG_RW_TXDONE
	UINT32 status;
	DD_HANDLE sdio_hdl;
	STM32_FRAME_HDR *hdr;
	int len = sizeof(*cfm) + sizeof(STM32_FRAME_HDR);
	u8 buf[sizeof(*cfm) + sizeof(STM32_FRAME_HDR)] __attribute__((aligned(4)));

	SDIO_PRT("cookie: %lx\n", cfm->cookie);
	if (!cfm->cookie)
		return SDIO_INTF_SUCCESS;
	hdr = (STM32_FRAME_HDR *)buf;

	hdr->len = len;
	hdr->type = MVMS_TXDONE;
	//hdr->seq = seq;

	// fill cfm
	cfm->credits = 1;
	os_memcpy(hdr + 1, cfm, sizeof(*cfm));

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl) {
		os_free(buf);
		return SDIO_INTF_FAILURE;
	}

	//print_hex_dump("CFM: ", buf, len);
	ddev_write(sdio_hdl, (char *)buf, len, S2H_WR_SYNC);
#endif

	return SDIO_INTF_SUCCESS;
}

void sdio_emb_rxed_evt(int dummy)
{
#if !CONFIG_RWNX_QOS_MSDU
	UINT32 status;
#endif
#if CFG_WMM_PATCH
	int drop = 0;
#endif
	UINT32 queue_idx;
	UINT8 *content_ptr;
	bool pushed = false;
	SDIO_NODE_PTR mem_node_ptr;
	struct stm32_data_tx_frame *frm_tx_ptr;
	STM32_FRAME_HDR *frm_hdr_ptr;
	UINT8 i = 0;
	struct sta_info_tag *sta;
	struct vif_info_tag *vif;

	queue_idx = AC_VI;
	mem_node_ptr = sdio_get_rxed_node();
	SDIO_PRT("%s %d\n", __func__, __LINE__);

	while (mem_node_ptr) {
		frm_hdr_ptr = (STM32_FRAME_HDR *)mem_node_ptr->addr;
		switch (frm_hdr_ptr->type) {
		case MVMS_DAT: {
			UINT8 tid;
			//ETHHDR_PTR eth_hdr_ptr;
			struct txdesc *txdesc_new;
			//GLOBAL_INT_DECLARATION();

			//GLOBAL_INT_DISABLE();
			//list_add_tail(&mem_node_ptr->node_list, &inbound_list);
			//GLOBAL_INT_RESTORE();

			frm_tx_ptr = (struct stm32_data_tx_frame *)mem_node_ptr->addr;
			frm_tx_ptr->cfm.addr = (uint32_t)mem_node_ptr;	/* reset addr */
			content_ptr = (UINT8 *)frm_tx_ptr->frm;

			if (sdio_debug_level) {
				print_hex_dump("FRM: ", (u8 *)frm_tx_ptr, frm_tx_ptr->hdr.len);
				//print_hex_dump("CONT: ", (u8 *)content_ptr, frm_tx_ptr->hdr.len - sizeof(*frm_tx_ptr));
			}
#if CONFIG_TEMP_DETECT
			if (temp_detct_get_cali_flag()) // check temperature
				goto rxed_exception;
#endif

#if !CONFIG_RWNX_QOS_MSDU
			status = sdio_emb_get_tx_info(content_ptr, &tid);
			if (SDIO_INTF_SUCCESS != status)
				goto rxed_exception;

			queue_idx = sdio_emb_get_hwqueue_id(tid);
#else
			tid = frm_tx_ptr->host.tid;
			vif = rwm_mgmt_vif_idx2ptr(frm_tx_ptr->host.vif_idx);
			if (likely(vif->active && tid != 0xFF)) {
				sta = &sta_info_tab[vif->u.sta.ap_id];
				if (sta && sta->info.capa_flags & STA_QOS_CAPA) {
					/* check admission ctrl */
					for (i = mac_tid2ac[tid]; i >= 0; i--)
						if (!(vif->bss_info.edca_param.acm & BIT(i)))
							break;
					if (i < 0)
						goto rxed_exception;
					//queue_idx = i;	/* AC_* */
					/*
					 * TBD: we only allocate a few count of tx decriptor for ACs that are not AC_VI under rwnx_config.h,
					 * which may cause memory corruption when tx using these ACs. So we use AC_VI to transmit frames or
					 * we largen tx descriptor number.
					 * Remember that transmit frames use the same ac doesn't conform to IEEE802.11 spec.
					 */
					queue_idx = AC_VI;
				} else {
					/*
					 * non-WMM STA
					 *
					 * CWmin 15, CWmax 1023, AIFSN 2, TXOP 0. set these values when joining with this BSS.
					 */
					tid = 0xFF;
					queue_idx = AC_VI;
				}
			} else {
				tid = 0xFF;
				queue_idx = AC_VI;
			}
			//bk_printf("%s: tid %d, q %d\n", __func__, tid, queue_idx);
#endif

			/*allocation of the tx descriptor*/
			txdesc_new = tx_txdesc_prepare(queue_idx);
			if (!txdesc_new || TXDESC_STA_USED == txdesc_new->status) {
				fatal_prf("TFull\r\n");
				frm_tx_ptr->cfm.credits = 1;
				inbound_cfm(&frm_tx_ptr->cfm);

				goto rxed_exception;
			}
			txdesc_new->status = TXDESC_STA_USED;

			//if (os_memcmp(&frm_tx_ptr->host.eth_dest_addr, "\xd8\xc4\x97\x81\xa0\x5a", 6) == 0)
			//	bk_printf("T\n");

			os_memcpy(&txdesc_new->host, &frm_tx_ptr->host, sizeof(struct hostdesc));
			//sdio_emb_txdesc_copy(txdesc_new, eth_hdr_ptr);
			os_memcpy(&txdesc_new->lmac.hw_desc->cfm, &frm_tx_ptr->cfm, sizeof(struct tx_cfm_tag));
			SDIO_PRT("hostid: %lx\n", txdesc_new->lmac.hw_desc->cfm.cookie);

			//override some memory addresses
			txdesc_new->host.orig_addr = (UINT32)mem_node_ptr->orig_addr;
			if (txdesc_new->host.flags & TXU_CNTRL_MGMT) {
				txdesc_new->host.packet_addr	  = (UINT32)content_ptr;
				txdesc_new->host.status_desc_addr = (UINT32)content_ptr;
			} else {
				txdesc_new->host.packet_addr      = (UINT32)content_ptr + 14;
				txdesc_new->host.status_desc_addr = (UINT32)content_ptr + 14;
			}
			txdesc_new->host.tid              = tid;		/* FIXME */

			// Initialize some fields of the descriptor
			txdesc_new->lmac.agg_desc = NULL;
			txdesc_new->lmac.hw_desc->cfm.status = 0;

			//print_hex_dump("host desc: ", (u8 *)&txdesc_new->host, sizeof(txdesc_new->host));
#if NX_POWERSAVE
			txl_cntrl_inc_pck_cnt();
#endif

			txu_cntrl_push(txdesc_new, queue_idx);

			pushed = true;

#if CONFIG_SDIO_BLOCK_512
			i++;
#endif
			break;
		}

		case MVMS_CMD: {
			UINT32 ret;
			struct ke_msg *kmsg;

			// ack host
			cmd_ack_host(frm_hdr_ptr->seq);

			ret = sdio_h2e_msg_conversion(frm_hdr_ptr, &kmsg);

			if (SDIO_INTF_SUCCESS == ret)
				sdio_h2e_kmsg_hdlr(kmsg);

			sdio_release_one_node(mem_node_ptr);

			break;
		}

		case MVMS_EVENT:
			break;

		case MVMS_TXDONE:
			break;

		default:
			os_printf("unknown type: %d\n", frm_hdr_ptr->type);
			break;
		}

		mem_node_ptr = sdio_get_rxed_node();

#if CONFIG_SDIO_BLOCK_512
		if (i > 10) {
			os_printf("sdio intf relax..\n");
			mem_node_ptr = 0;
		}
#endif
	}

	if (0 == mem_node_ptr)
		ke_evt_clear(KE_EVT_SDIO_RXED_DATA_BIT);

	if (pushed) {
#if (NX_AMPDU_TX)
		// Check if the current A-MPDU under construction has to be closed
		txl_aggregate_check(queue_idx);
#endif //(NX_AMPDU_TX)
	}

rxed_exception:

	return;
}

void sdio_rxed_trigger_evt(void)
{
	SDIO_PRT("%s %d\n", __func__, __LINE__);
	ke_evt_set(KE_EVT_SDIO_RXED_DATA_BIT);
}

UINT32 sdio_intf_init(void)
{
	UINT32 ret;
	UINT32 status;
	DD_HANDLE sdio_hdl;
	RW_CONNECTOR_T intf;

	ret = SDIO_INTF_FAILURE;

	intf.msg_outbound_func = sdio_emb_kmsg_fwd;
	intf.data_outbound_func = outbound_upload_data;
	intf.rx_alloc_func = sdio_get_free_node;
	intf.get_rx_valid_status_func = sdio_get_free_node_count;
	intf.tx_confirm_func = inbound_cfm;
	rwnxl_register_connector(&intf);

	INIT_LIST_HEAD(&inbound_list);

	sdio_hdl = ddev_open(DD_DEV_TYPE_SDIO, &status, 0);
	if (DD_HANDLE_UNVALID == sdio_hdl)
		goto init_exception;

	ddev_control(sdio_hdl, SDIO_CMD_REG_RX_CALLBACK, (void *)sdio_rxed_trigger_evt);

	//SDIO_INTF_PRT("sdio_intf_init\r\n");
	ret = SDIO_INTF_SUCCESS;

init_exception:

	return ret;
}
#endif // CONFIG_SDIO
// EOF


