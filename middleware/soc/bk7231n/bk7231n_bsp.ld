/* 
 * Script for GNU linker.
 * Describes layout of sections, location of stack.
 *
 * In this case vectors are at location 0 (reset @ 0x08)
 *
 * +------------+  0x00400020
 * data         |
 * end
 * |(heap)      |
 * .            .
 * .            .
 * |(heap limit)|
 *
 * |- - - - - - |
 * stack bottom    256k
 * +------------+  
 *
 * +------------+  0x0000000
 * |vectors     |
 * |            |
 * |------------+
 * |text        |  
 * |data        |
 * |            |  1024k
 * +------------+
 */


/* Split memory into area for vectors and ram */
MEMORY
{
        flash  (rx) : ORIGIN = 0x00010000, LENGTH = 3960K
        tcm   (rw!x): ORIGIN = 0x003F0000, LENGTH = 60k
        itcm   (rwx): ORIGIN = 0x003FF000, LENGTH = 4k
        ram   (rw!x): ORIGIN = 0x00400100, LENGTH = 192k - 0x100
}

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_vector_start);
_vector_start = ORIGIN(flash);

SECTIONS
{
/* vectors go to vectors region */
	. = ORIGIN(flash);
	.vectors : 
	{ 
		KEEP(*(*.vectors))
		KEEP( *(*.boot))
	} > flash

	/* this is for c++ global constructor init */
	.init_array :
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array*))
		PROVIDE_HIDDEN (__init_array_end = .);
	} > flash

/* instructions go to the text region*/

    . = ORIGIN(itcm);
    .itcm.code ALIGN(8) :
    {
        /* itcm 4KB code */
        *(.text.intc_hdl_entry)
        *(.text.intc_irq)
        *(.text.intc_fiq)
        *(.text.bk_timer_isr)
        *(.text.power_save_wakeup_isr)
        *(.text.bmsg_rx_sender)
        *(.text.bmsg_null_sender)
        *(.text.bk_get_tick)
        *(.text.flash_read_sr)
        *(.text.flash_write_sr)
        *(.text.flash_clr_qwfr)
        *(.text.set_flash_protect)
        *(.text.flash_read)
        *(.text.flash_read_data)
        *(.text.flash_set_qe)
        *(.text.flash_set_qwfr)
        *(.text.flash_set_line_mode*)
        *(.text.flash_get_line_mode)
        *(.text.flash_write)
        *(.text.flash_ctrl)
        *(.text.power_save_dtim_wake)
		*(.text.sctrl_fix_dpll_div)

        *(.text.vTaskSuspendAll)
        *(.text.xTaskGetTickCount)
        *(.text.xTaskGetTickCountFromISR)
        *(.text.vTaskStepTick)
        *(.text.xTaskIncrementTick)
        *(.text.xTaskResumeAll)
        *(.text.vTaskSwitchContext)
        *(.text.vApplicationIdleHook)
        *(.text.platform_is_in_irq_context)
        *(.text.platform_is_in_fiq_context)
        *(.text.platform_is_in_interrupt_context)
        *(.text.portENABLE_IRQ)
        *(.text.portENABLE_FIQ)
        *(.text.portDISABLE_FIQ)
        *(.text.portDISABLE_IRQ)
        *(.text.vPortEnterCritical)
        *(.text.vPortExitCritical)
    } > itcm AT>flash
    _itcmcode_flash_begin = LOADADDR(.itcm.code);
    _itcmcode_ram_begin = ADDR(.itcm.code);
    _itcmcode_ram_end = _itcmcode_ram_begin + SIZEOF(.itcm.code);

    . = ALIGN(0x8);
/* code, instructions.for example: i=i+1; */
	.text : 
	{
	    *(.text)
	    *(.text.*)
	    *(.stub)
	    /* .gnu.warning sections are handled specially by elf32.em.  */
	    *(.gnu.warning)
	    *(.gnu.linkonce.t*)
	    *(.glue_7t) *(.glue_7)
            __flash_txt_end = .;
	} > flash

/* read only data.for example: const int rom_data[3]={1,2,3}; */
	.rodata ALIGN(8) :
	{
		*(.rodata)
		*(.rodata.*)
		*(.gnu.linkonce.r*)
	} > flash
	
	__exidx_start = .;
	.ARM.exidx :
	{
	    *(.ARM.exidx*)
	    *(.gnu.linkonce.armexidx.*)
	} > flash
	__exidx_end = .;
	
    . = ORIGIN(tcm);
    .tcm ALIGN(8) :
    {
		/* BEKEN */
		*mem_arch.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*hostapd*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*bk_cal*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rwnx_intf*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*power_save*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*wlan_ui*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ate_app*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*arch_main*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*irda*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*spi*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*wlan_cli*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rtos_pub*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*phy_trident*.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sa_station.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ieee802_11_demo.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dd.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dma.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*wpa_psk_cache.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*pwm_bk7231n.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*wpa_debug.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ctrl_iface.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*wpa_psk_cache.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*arbitrate.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)

		/* driver */
		*drv_model.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*intc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*role_launch.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*manual_cal_bk7231U.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*main_none.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*eloop.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*main_supplicant.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*wpa_supplicant.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rw_ieee80211.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rw_msg_rx.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*temp_detect.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*flash.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gpio.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*mac_phy_bypass.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*bk_timer.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*saradc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sys_ctrl.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*BkDriverFlash.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*cmd_rx_sensitivity.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*cmd_evm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rx_sensitivity.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*start_type.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*common.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)

		/* beken_sdk */
		*bk7231N_cal.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*fake_clock.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*tx_evm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*param_config.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sa_ap.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*apm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*apm_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*bam_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*bam.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*chan.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*hal_dma.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*hal_machw.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ke_env.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*me_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ps.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rwnx.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rx_swdesc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rxu_cntrl.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rxl_cntrl.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rxl_hwdesc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sta_mgmt.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*scan_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*scan.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*scanu.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*scanu_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sm_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*me.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*mm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*mm_bcn.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*mm_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*mm_timer.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*td.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		/* *tx_swdesc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON) */
		*txl_buffer.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*txl_cfm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*txl_cntrl.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*vif_mgmt.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*wdt.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*app.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)

		/* beken_ble */
		*ble_rf_port.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rwip.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gapc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gapc_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gapm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gapm_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gattc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gattc_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gattm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*gattm_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*l2cc_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*l2cm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*llc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*lld.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*llm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*hci.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sch_slice.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ble_aes.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ecc_p256.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*kernel.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rwip_driver.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*l2cc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ble_util_buf.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*llc_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*lld_adv.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*lld_con.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*lld_per_adv.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*lld_test.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*llm_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*hci_fc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sch_alarm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sch_arb.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sch_plan.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sch_prog.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*kernel_event.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*kernel_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*app_ble.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ble_ui.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*hci_tl.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*h4tl.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sdp_service.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*app_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*app_sdp.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ble.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*prf.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*comm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*comm_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sdp_service_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*app_comm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*app_sec.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*common_list.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*common_utils.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*RomCallFlash.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dbg.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dbg_mwsgen.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dbg_swdiag.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dbg_task.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ble_rf_xvr.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rwip.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*uart.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*prf_utils.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rf_xvr.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*uart_ble.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ble_main.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*rwble.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*app_ble_init.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sdp_common.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*lld_init.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)

		/* don't add it to tcm */
		*hal_desc.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		/* *scanu_shared.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*txl_buffer_shared.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*txl_frame_shared.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*scan_shared.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)*/

		/* LWIP */
		*dhcp-server-main.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dhcp-server.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*timeouts.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*tcp_in.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*tcp.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*raw.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*netif.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		/* *memp.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON) */
		/* *mem.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON) */
		*mem.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ip4.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ip4_addr.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*igmp.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*etharp.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dhcp.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*dns.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ping.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*tcpip.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sockets.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*net.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sys_arch.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*nd6.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*icmp6.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*api_lib.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ethernetif.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*netbuf.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*api_msg.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*netifapi.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*tcpip.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ip.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ethip6.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*icmp6.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ip6_addr.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ip6.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ip6_frag.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*mld6.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*stats.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*pbuf.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*sys.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*tcp_out.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*udp.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*ethernet.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)

		/* OS */
		*tasks.marm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*heap_4.marm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*timers.marm.c.obj(.bss .bss.* .scommon .sbss .dynbss COMMON)

    } >tcm AT>flash
    _tcmbss_start = ADDR(.tcm);
    _tcmbss_end = _tcmbss_start + SIZEOF(.tcm);

    . = ORIGIN(ram);
/* globals.for example: int ram_data[3]={4,5,6}; */		/* VMA in RAM, but keep LMA in flash */
	_begin_data = .;
        .data :
	{
	    *(.data .data.*)
	    *(.sdata) 
	    *(.gnu.linkonce.d*)
    	SORT(CONSTRUCTORS)
        } >ram AT>flash
        _end_data = .;
	
	/* Loader will copy data from _flash_begin to _ram_begin..ram_end */
	_data_flash_begin = LOADADDR(.data);
	_data_ram_begin = ADDR(.data);
	_data_ram_end = .;

/* uninitialized data section - global   int i; */
	.bss ALIGN(8):
	{
		_bss_start = .;
		*boot_handlers.O(.bss .bss.* .scommon .sbss .dynbss COMMON)
		*(.bss .bss.*)
		*(.scommon)
		*(.sbss)
		*(.dynbss)
		*(COMMON)
		/* Align here to ensure that the .bss section occupies space up to
		_end.  Align after .bss to ensure correct alignment even if the
		.bss section disappears because there are no input sections.  */
		. = ALIGN(32 / 8);
		_bss_end = .;
	} > ram						/* in RAM */

	. = ALIGN (8);
	_empty_ram = .;

/* This symbol defines end of code/data sections. Heap starts here. */
	PROVIDE(end    	  = .);
}
 GROUP(
   libgcc.a
   libg.a
   libc.a
   libm.a
   libnosys.a
 )
