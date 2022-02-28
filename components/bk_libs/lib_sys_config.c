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


#include "sys_config.h"
#include "drv_model_pub.h"
#include "stdint.h"
#include "stdbool.h"


uint32_t lib_config_sdio_credits_get(void)
{
#ifdef CONFIG_SDIO_CREDITS
	return CONFIG_SDIO_CREDITS;
#else
	return 0;
#endif
}

uint32_t lib_config_camera_get(void)
{
#ifdef CONFIG_CAMERA
	return CONFIG_CAMERA;
#else
	return 0;
#endif
}

uint32_t lib_config_spi_master_get(void)
{
#ifdef CONFIG_SPI_MASTER
	return CONFIG_SPI_MASTER;
#else
	return 0;
#endif
}

uint32_t lib_config_beacon_vendor_api_get(void)
{
#ifdef CONFIG_BEACON_VENDOR_API
	return CONFIG_BEACON_VENDOR_API;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_saradc_get(void)
{
#ifdef DD_DEV_TYPE_SARADC
	return DD_DEV_TYPE_SARADC;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_ps_get(void)
{
#ifdef CONFIG_BLE_PS
	return CONFIG_BLE_PS;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_uart2_get(void)
{
#ifdef DD_DEV_TYPE_UART2
	return DD_DEV_TYPE_UART2;
#else
	return 0;
#endif
}

uint32_t lib_config_wpa3_enterprise_get(void)
{
#ifdef CONFIG_WPA3_ENTERPRISE
	return CONFIG_WPA3_ENTERPRISE;
#else
	return 0;
#endif
}

uint32_t lib_config_temp_detect_get(void)
{
#ifdef CONFIG_TEMP_DETECT
	return CONFIG_TEMP_DETECT;
#else
	return 0;
#endif
}

uint32_t lib_config_mac_phy_bypass_get(void)
{
#ifdef CONFIG_MAC_PHY_BYPASS
	return CONFIG_MAC_PHY_BYPASS;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_aud_dac_get(void)
{
#ifdef DD_DEV_TYPE_AUD_DAC
	return DD_DEV_TYPE_AUD_DAC;
#else
	return 0;
#endif
}

uint32_t lib_config_soc_bk7236_get(void)
{
#ifdef CONFIG_SOC_BK7236
	return CONFIG_SOC_BK7236;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi6_ip_debug_get(void)
{
#ifdef CONFIG_WIFI6_IP_DEBUG
	return CONFIG_WIFI6_IP_DEBUG;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_dsp_get(void)
{
#ifdef DD_DEV_TYPE_DSP
	return DD_DEV_TYPE_DSP;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_p2p_go_get(void)
{
#ifdef CONFIG_WIFI_P2P_GO
	return CONFIG_WIFI_P2P_GO;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_mesh_rw_get(void)
{
#ifdef CONFIG_BLE_MESH_RW
	return CONFIG_BLE_MESH_RW;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_mpb_get(void)
{
#ifdef DD_DEV_TYPE_MPB
	return DD_DEV_TYPE_MPB;
#else
	return 0;
#endif
}

uint32_t lib_config_dhcp_get(void)
{
#ifdef CONFIG_DHCP
	return CONFIG_DHCP;
#else
	return 0;
#endif
}

uint32_t lib_config_cali_get(void)
{
#ifdef CONFIG_CALI
	return CONFIG_CALI;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_sctrl_get(void)
{
#ifdef DD_DEV_TYPE_SCTRL
	return DD_DEV_TYPE_SCTRL;
#else
	return 0;
#endif
}

uint32_t lib_config_rf_ota_test_get(void)
{
#ifdef CONFIG_RF_OTA_TEST
	return CONFIG_RF_OTA_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_background_print_get(void)
{
#ifdef CONFIG_BACKGROUND_PRINT
	return CONFIG_BACKGROUND_PRINT;
#else
	return 0;
#endif
}

uint32_t lib_config_task_usb_prio_get(void)
{
#ifdef CONFIG_TASK_USB_PRIO
	return CONFIG_TASK_USB_PRIO;
#else
	return 0;
#endif
}

uint32_t lib_config_uart_debug_get(void)
{
#ifdef CONFIG_UART_DEBUG
	return CONFIG_UART_DEBUG;
#else
	return 0;
#endif
}

uint32_t lib_config_fpga_get(void)
{
#ifdef CONFIG_FPGA
	return CONFIG_FPGA;
#else
	return 0;
#endif
}

uint32_t lib_config_rf_policy_ble_req_get(void)
{
#ifdef CONFIG_RF_POLICY_BLE_REQ
	return CONFIG_RF_POLICY_BLE_REQ;
#else
	return 0;
#endif
}

uint32_t lib_config_hslave_spi_get(void)
{
#ifdef CONFIG_HSLAVE_SPI
	return CONFIG_HSLAVE_SPI;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_host_get(void)
{
#ifdef CONFIG_USB_HOST
	return CONFIG_USB_HOST;
#else
	return 0;
#endif
}

uint32_t lib_config_msdu_resv_head_len_get(void)
{
#ifdef CONFIG_MSDU_RESV_HEAD_LEN
	return CONFIG_MSDU_RESV_HEAD_LEN;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_sdio_get(void)
{
#ifdef DD_DEV_TYPE_SDIO
	return DD_DEV_TYPE_SDIO;
#else
	return 0;
#endif
}

uint32_t lib_config_int_wdt_period_ms_get(void)
{
#ifdef CONFIG_INT_WDT_PERIOD_MS
	return CONFIG_INT_WDT_PERIOD_MS;
#else
	return 0;
#endif
}

uint32_t lib_config_malloc_statis_get(void)
{
#ifdef CONFIG_MALLOC_STATIS
	return CONFIG_MALLOC_STATIS;
#else
	return 0;
#endif
}

uint32_t lib_config_bt_src_add_get(void)
{
#ifdef CONFIG_BT_SRC_ADD
	return CONFIG_BT_SRC_ADD;
#else
	return 0;
#endif
}

uint32_t lib_config_uart3_get(void)
{
#ifdef CONFIG_UART3
	return CONFIG_UART3;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_raw_tx_test_get(void)
{
#ifdef CONFIG_WIFI_RAW_TX_TEST
	return CONFIG_WIFI_RAW_TX_TEST;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_irda_get(void)
{
#ifdef DD_DEV_TYPE_IRDA
	return DD_DEV_TYPE_IRDA;
#else
	return 0;
#endif
}

uint32_t lib_config_peri_test_get(void)
{
#ifdef CONFIG_PERI_TEST
	return CONFIG_PERI_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_wpa_ctrl_iface_get(void)
{
#ifdef CONFIG_WPA_CTRL_IFACE
	return CONFIG_WPA_CTRL_IFACE;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_usb_plug_get(void)
{
#ifdef DD_DEV_TYPE_USB_PLUG
	return DD_DEV_TYPE_USB_PLUG;
#else
	return 0;
#endif
}

uint32_t lib_config_fake_rtc_ps_get(void)
{
#ifdef CONFIG_FAKE_RTC_PS
	return CONFIG_FAKE_RTC_PS;
#else
	return 0;
#endif
}

uint32_t lib_config_rwnx_qos_msdu_get(void)
{
#ifdef CONFIG_RWNX_QOS_MSDU
	return CONFIG_RWNX_QOS_MSDU;
#else
	return 0;
#endif
}

uint32_t lib_config_spi_slave_get(void)
{
#ifdef CONFIG_SPI_SLAVE
	return CONFIG_SPI_SLAVE;
#else
	return 0;
#endif
}

uint32_t lib_config_soc_bk7271_get(void)
{
#ifdef CONFIG_SOC_BK7271
	return CONFIG_SOC_BK7271;
#else
	return 0;
#endif
}

uint32_t lib_config_fft_get(void)
{
#ifdef CONFIG_FFT
	return CONFIG_FFT;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_uvc_get(void)
{
#ifdef CONFIG_USB_UVC
	return CONFIG_USB_UVC;
#else
	return 0;
#endif
}

uint32_t lib_config_no_hosted_get(void)
{
#ifdef CONFIG_NO_HOSTED
	return CONFIG_NO_HOSTED;
#else
	return 0;
#endif
}

uint32_t lib_config_uart_test_get(void)
{
#ifdef CONFIG_UART_TEST
	return CONFIG_UART_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_mp3player_get(void)
{
#ifdef CONFIG_MP3PLAYER
	return CONFIG_MP3PLAYER;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_uart3_get(void)
{
#ifdef DD_DEV_TYPE_UART3
	return DD_DEV_TYPE_UART3;
#else
	return 0;
#endif
}

uint32_t lib_config_bssid_connect_get(void)
{
#ifdef CONFIG_BSSID_CONNECT
	return CONFIG_BSSID_CONNECT;
#else
	return 0;
#endif
}

uint32_t lib_config_bt_mesh_cfg_cli_get(void)
{
#ifdef CONFIG_BT_MESH_CFG_CLI
	return CONFIG_BT_MESH_CFG_CLI;
#else
	return 0;
#endif
}

uint32_t lib_config_wpa_log_get(void)
{
#ifdef CONFIG_WPA_LOG
	return CONFIG_WPA_LOG;
#else
	return 0;
#endif
}

uint32_t lib_config_lwip_get(void)
{
#ifdef CONFIG_LWIP
	return CONFIG_LWIP;
#else
	return 0;
#endif
}

uint32_t lib_config_manual_cali_get(void)
{
#ifdef CONFIG_MANUAL_CALI
	return CONFIG_MANUAL_CALI;
#else
	return 0;
#endif
}

uint32_t lib_config_task_wpas_prio_get(void)
{
#ifdef CONFIG_TASK_WPAS_PRIO
	return CONFIG_TASK_WPAS_PRIO;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi6_code_stack_get(void)
{
#ifdef CONFIG_WIFI6_CODE_STACK
	return CONFIG_WIFI6_CODE_STACK;
#else
	return 0;
#endif
}

uint32_t lib_config_sdcard_host_get(void)
{
#ifdef CONFIG_SDCARD_HOST
	return CONFIG_SDCARD_HOST;
#else
	return 0;
#endif
}

uint32_t lib_config_soc_bk7231n_get(void)
{
#ifdef CONFIG_SOC_BK7231N
	return CONFIG_SOC_BK7231N;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_device_get(void)
{
#ifdef CONFIG_USB_DEVICE
	return CONFIG_USB_DEVICE;
#else
	return 0;
#endif
}

uint32_t lib_config_udisk_mp3_get(void)
{
#ifdef CONFIG_UDISK_MP3
	return CONFIG_UDISK_MP3;
#else
	return 0;
#endif
}

uint32_t lib_config_use_conv_utf8_get(void)
{
#ifdef CONFIG_USE_CONV_UTF8
	return CONFIG_USE_CONV_UTF8;
#else
	return 0;
#endif
}

uint32_t lib_config_bt_get(void)
{
#ifdef CONFIG_BT
	return CONFIG_BT;
#else
	return 0;
#endif
}

uint32_t lib_config_rf_policy_wifi_req_get(void)
{
#ifdef CONFIG_RF_POLICY_WIFI_REQ
	return CONFIG_RF_POLICY_WIFI_REQ;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_fft_get(void)
{
#ifdef DD_DEV_TYPE_FFT
	return DD_DEV_TYPE_FFT;
#else
	return 0;
#endif
}

uint32_t lib_config_task_wdt_get(void)
{
#ifdef CONFIG_TASK_WDT
	return CONFIG_TASK_WDT;
#else
	return 0;
#endif
}

uint32_t lib_config_uart1_get(void)
{
#ifdef CONFIG_UART1
	return CONFIG_UART1;
#else
	return 0;
#endif
}

uint32_t lib_config_customer_drone_get(void)
{
#ifdef CONFIG_CUSTOMER_DRONE
	return CONFIG_CUSTOMER_DRONE;
#else
	return 0;
#endif
}

uint32_t lib_config_i2s_get(void)
{
#ifdef CONFIG_I2S
	return CONFIG_I2S;
#else
	return 0;
#endif
}

uint32_t lib_config_bt_mesh_adv_legacy_get(void)
{
#ifdef CONFIG_BT_MESH_ADV_LEGACY
	return CONFIG_BT_MESH_ADV_LEGACY;
#else
	return 0;
#endif
}

uint32_t lib_config_task_wdt_period_ms_get(void)
{
#ifdef CONFIG_TASK_WDT_PERIOD_MS
	return CONFIG_TASK_WDT_PERIOD_MS;
#else
	return 0;
#endif
}

uint32_t lib_config_spi_mst_flash_get(void)
{
#ifdef CONFIG_SPI_MST_FLASH
	return CONFIG_SPI_MST_FLASH;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_icu_get(void)
{
#ifdef DD_DEV_TYPE_ICU
	return DD_DEV_TYPE_ICU;
#else
	return 0;
#endif
}

uint32_t lib_config_ap_he_get(void)
{
#ifdef CONFIG_AP_HE
	return CONFIG_AP_HE;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi6_get(void)
{
#ifdef CONFIG_WIFI6
	return CONFIG_WIFI6;
#else
	return 0;
#endif
}

uint32_t lib_config_lwip_mem_reduce_get(void)
{
#ifdef CONFIG_LWIP_MEM_REDUCE
	return CONFIG_LWIP_MEM_REDUCE;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_kmsg_task_stack_size_get(void)
{
#ifdef CONFIG_WIFI_KMSG_TASK_STACK_SIZE
	return CONFIG_WIFI_KMSG_TASK_STACK_SIZE;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_get(void)
{
#ifdef CONFIG_USB
	return CONFIG_USB;
#else
	return 0;
#endif
}

uint32_t lib_config_pmf_get(void)
{
#ifdef CONFIG_PMF
	return CONFIG_PMF;
#else
	return 0;
#endif
}

uint32_t lib_config_tick_cali_get(void)
{
#ifdef CONFIG_TICK_CALI
	return CONFIG_TICK_CALI;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_mailbox_get(void)
{
#ifdef DD_DEV_TYPE_MAILBOX
	return DD_DEV_TYPE_MAILBOX;
#else
	return 0;
#endif
}

uint32_t lib_config_usb1_port_get(void)
{
#ifdef CONFIG_USB1_PORT
	return CONFIG_USB1_PORT;
#else
	return 0;
#endif
}

uint32_t lib_config_spi_test_get(void)
{
#ifdef CONFIG_SPI_TEST
	return CONFIG_SPI_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_host_rw_get(void)
{
#ifdef CONFIG_BLE_HOST_RW
	return CONFIG_BLE_HOST_RW;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_core_task_stack_size_get(void)
{
#ifdef CONFIG_WIFI_CORE_TASK_STACK_SIZE
	return CONFIG_WIFI_CORE_TASK_STACK_SIZE;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_conn_num_get(void)
{
#ifdef CONFIG_BLE_CONN_NUM
	return CONFIG_BLE_CONN_NUM;
#else
	return 0;
#endif
}

uint32_t lib_config_uart2_get(void)
{
#ifdef CONFIG_UART2
	return CONFIG_UART2;
#else
	return 0;
#endif
}

uint32_t lib_config_wfa_cert_get(void)
{
#ifdef CONFIG_WFA_CERT
	return CONFIG_WFA_CERT;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_5_x_get(void)
{
#ifdef CONFIG_BLE_5_X
	return CONFIG_BLE_5_X;
#else
	return 0;
#endif
}

uint32_t lib_config_trng_support_get(void)
{
#ifdef CONFIG_TRNG_SUPPORT
	return CONFIG_TRNG_SUPPORT;
#else
	return 0;
#endif
}

uint32_t lib_config_wpa_sme_get(void)
{
#ifdef CONFIG_WPA_SME
	return CONFIG_WPA_SME;
#else
	return 0;
#endif
}

uint32_t lib_config_at_get(void)
{
#ifdef CONFIG_AT
	return CONFIG_AT;
#else
	return 0;
#endif
}

uint32_t lib_config_mem_debug_get(void)
{
#ifdef CONFIG_MEM_DEBUG
	return CONFIG_MEM_DEBUG;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_mesh_zephyr_get(void)
{
#ifdef CONFIG_BLE_MESH_ZEPHYR
	return CONFIG_BLE_MESH_ZEPHYR;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_ccd_get(void)
{
#ifdef CONFIG_USB_CCD
	return CONFIG_USB_CCD;
#else
	return 0;
#endif
}

uint32_t lib_config_ap_idle_get(void)
{
#ifdef CONFIG_AP_IDLE
	return CONFIG_AP_IDLE;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_sensor_get(void)
{
#ifdef CONFIG_WIFI_SENSOR
	return CONFIG_WIFI_SENSOR;
#else
	return 0;
#endif
}

uint32_t lib_config_int_wdt_get(void)
{
#ifdef CONFIG_INT_WDT
	return CONFIG_INT_WDT;
#else
	return 0;
#endif
}

uint32_t lib_config_beacon_update_api_get(void)
{
#ifdef CONFIG_BEACON_UPDATE_API
	return CONFIG_BEACON_UPDATE_API;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_timer_get(void)
{
#ifdef DD_DEV_TYPE_TIMER
	return DD_DEV_TYPE_TIMER;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_ejpeg_get(void)
{
#ifdef DD_DEV_TYPE_EJPEG
	return DD_DEV_TYPE_EJPEG;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_kmsg_task_prio_get(void)
{
#ifdef CONFIG_WIFI_KMSG_TASK_PRIO
	return CONFIG_WIFI_KMSG_TASK_PRIO;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_core_task_prio_get(void)
{
#ifdef CONFIG_WIFI_CORE_TASK_PRIO
	return CONFIG_WIFI_CORE_TASK_PRIO;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_uart1_get(void)
{
#ifdef DD_DEV_TYPE_UART1
	return DD_DEV_TYPE_UART1;
#else
	return 0;
#endif
}

uint32_t lib_config_wpa2_enterprise_get(void)
{
#ifdef CONFIG_WPA2_ENTERPRISE
	return CONFIG_WPA2_ENTERPRISE;
#else
	return 0;
#endif
}

uint32_t lib_config_general_dma_get(void)
{
#ifdef CONFIG_GENERAL_DMA
	return CONFIG_GENERAL_DMA;
#else
	return 0;
#endif
}

uint32_t lib_config_security_get(void)
{
#ifdef CONFIG_SECURITY
	return CONFIG_SECURITY;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_qspi_get(void)
{
#ifdef DD_DEV_TYPE_QSPI
	return DD_DEV_TYPE_QSPI;
#else
	return 0;
#endif
}

uint32_t lib_config_semi_hosted_get(void)
{
#ifdef CONFIG_SEMI_HOSTED
	return CONFIG_SEMI_HOSTED;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_usb_get(void)
{
#ifdef DD_DEV_TYPE_USB
	return DD_DEV_TYPE_USB;
#else
	return 0;
#endif
}

uint32_t lib_config_tpc_pa_map_get(void)
{
#ifdef CONFIG_TPC_PA_MAP
	return CONFIG_TPC_PA_MAP;
#else
	return 0;
#endif
}

uint32_t lib_config_iperf_test_get(void)
{
#ifdef CONFIG_IPERF_TEST
	return CONFIG_IPERF_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_fully_hosted_get(void)
{
#ifdef CONFIG_FULLY_HOSTED
	return CONFIG_FULLY_HOSTED;
#else
	return 0;
#endif
}

uint32_t lib_config_soc_bk7231u_get(void)
{
#ifdef CONFIG_SOC_BK7231U
	return CONFIG_SOC_BK7231U;
#else
	return 0;
#endif
}

uint32_t lib_config_sta_ps_get(void)
{
#ifdef CONFIG_STA_PS
	return CONFIG_STA_PS;
#else
	return 0;
#endif
}

uint32_t lib_config_spi_dma_get(void)
{
#ifdef CONFIG_SPI_DMA
	return CONFIG_SPI_DMA;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_rf_get(void)
{
#ifdef DD_DEV_TYPE_RF
	return DD_DEV_TYPE_RF;
#else
	return 0;
#endif
}

uint32_t lib_config_ap_monitor_coexist_get(void)
{
#ifdef CONFIG_AP_MONITOR_COEXIST
	return CONFIG_AP_MONITOR_COEXIST;
#else
	return 0;
#endif
}

uint32_t lib_config_ap_ht_ie_get(void)
{
#ifdef CONFIG_AP_HT_IE
	return CONFIG_AP_HT_IE;
#else
	return 0;
#endif
}

uint32_t lib_config_pta_get(void)
{
#ifdef CONFIG_PTA
	return CONFIG_PTA;
#else
	return 0;
#endif
}

uint32_t lib_config_base_mac_from_nvs_get(void)
{
#ifdef CONFIG_BASE_MAC_FROM_NVS
	return CONFIG_BASE_MAC_FROM_NVS;
#else
	return 0;
#endif
}

uint32_t lib_config_wfa_ca_get(void)
{
#ifdef CONFIG_WFA_CA
	return CONFIG_WFA_CA;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_p2p_get(void)
{
#ifdef CONFIG_WIFI_P2P
	return CONFIG_WIFI_P2P;
#else
	return 0;
#endif
}

uint32_t lib_config_deep_ps_get(void)
{
#ifdef CONFIG_DEEP_PS
	return CONFIG_DEEP_PS;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_i2s_get(void)
{
#ifdef DD_DEV_TYPE_I2S
	return DD_DEV_TYPE_I2S;
#else
	return 0;
#endif
}

uint32_t lib_config_xtal_freq_40m_get(void)
{
#ifdef CONFIG_XTAL_FREQ_40M
	return CONFIG_XTAL_FREQ_40M;
#else
	return 0;
#endif
}

uint32_t lib_config_debug_firmware_get(void)
{
#ifdef CONFIG_DEBUG_FIRMWARE
	return CONFIG_DEBUG_FIRMWARE;
#else
	return 0;
#endif
}

uint32_t lib_config_lwip_2_1_2_get(void)
{
#ifdef CONFIG_LWIP_2_1_2
	return CONFIG_LWIP_2_1_2;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_hid_get(void)
{
#ifdef CONFIG_USB_HID
	return CONFIG_USB_HID;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_end_get(void)
{
#ifdef DD_DEV_TYPE_END
	return DD_DEV_TYPE_END;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_msd_get(void)
{
#ifdef CONFIG_USB_MSD
	return CONFIG_USB_MSD;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_spidma_get(void)
{
#ifdef DD_DEV_TYPE_SPIDMA
	return DD_DEV_TYPE_SPIDMA;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_flash_get(void)
{
#ifdef DD_DEV_TYPE_FLASH
	return DD_DEV_TYPE_FLASH;
#else
	return 0;
#endif
}

uint32_t lib_config_wpa3_get(void)
{
#ifdef CONFIG_WPA3
	return CONFIG_WPA3;
#else
	return 0;
#endif
}

uint32_t lib_config_xtal_freq_26m_get(void)
{
#ifdef CONFIG_XTAL_FREQ_26M
	return CONFIG_XTAL_FREQ_26M;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_scan_num_get(void)
{
#ifdef CONFIG_BLE_SCAN_NUM
	return CONFIG_BLE_SCAN_NUM;
#else
	return 0;
#endif
}

uint32_t lib_config_xtal_freq_get(void)
{
#ifdef CONFIG_XTAL_FREQ
	return CONFIG_XTAL_FREQ;
#else
	return 0;
#endif
}

uint32_t lib_config_dcache_get(void)
{
#ifdef CONFIG_DCACHE
	return CONFIG_DCACHE;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_ble_get(void)
{
#ifdef DD_DEV_TYPE_BLE
	return DD_DEV_TYPE_BLE;
#else
	return 0;
#endif
}

uint32_t lib_config_dsp_src_add_get(void)
{
#ifdef CONFIG_DSP_SRC_ADD
	return CONFIG_DSP_SRC_ADD;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_gdma_get(void)
{
#ifdef DD_DEV_TYPE_GDMA
	return DD_DEV_TYPE_GDMA;
#else
	return 0;
#endif
}

uint32_t lib_config_ota_tftp_get(void)
{
#ifdef CONFIG_OTA_TFTP
	return CONFIG_OTA_TFTP;
#else
	return 0;
#endif
}

uint32_t lib_config_msdu_resv_tail_len_get(void)
{
#ifdef CONFIG_MSDU_RESV_TAIL_LEN
	return CONFIG_MSDU_RESV_TAIL_LEN;
#else
	return 0;
#endif
}

uint32_t lib_config_bk_assert_reboot_get(void)
{
#ifdef CONFIG_BK_ASSERT_REBOOT
	return CONFIG_BK_ASSERT_REBOOT;
#else
	return 0;
#endif
}

uint32_t lib_config_spi_mst_psram_get(void)
{
#ifdef CONFIG_SPI_MST_PSRAM
	return CONFIG_SPI_MST_PSRAM;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_pwm_get(void)
{
#ifdef DD_DEV_TYPE_PWM
	return DD_DEV_TYPE_PWM;
#else
	return 0;
#endif
}

uint32_t lib_config_bkreg_get(void)
{
#ifdef CONFIG_BKREG
	return CONFIG_BKREG;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_bt_get(void)
{
#ifdef DD_DEV_TYPE_BT
	return DD_DEV_TYPE_BT;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_trng_get(void)
{
#ifdef DD_DEV_TYPE_TRNG
	return DD_DEV_TYPE_TRNG;
#else
	return 0;
#endif
}

uint32_t lib_config_demo_test_get(void)
{
#ifdef CONFIG_DEMO_TEST
	return CONFIG_DEMO_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_task_lwip_prio_get(void)
{
#ifdef CONFIG_TASK_LWIP_PRIO
	return CONFIG_TASK_LWIP_PRIO;
#else
	return 0;
#endif
}

uint32_t lib_config_i2c2_test_get(void)
{
#ifdef CONFIG_I2C2_TEST
	return CONFIG_I2C2_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_rf_policy_co_req_get(void)
{
#ifdef CONFIG_RF_POLICY_CO_REQ
	return CONFIG_RF_POLICY_CO_REQ;
#else
	return 0;
#endif
}

uint32_t lib_config_airkiss_test_get(void)
{
#ifdef CONFIG_AIRKISS_TEST
	return CONFIG_AIRKISS_TEST;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_spi_get(void)
{
#ifdef DD_DEV_TYPE_SPI
	return DD_DEV_TYPE_SPI;
#else
	return 0;
#endif
}

uint32_t lib_config_ap_vht_get(void)
{
#ifdef CONFIG_AP_VHT
	return CONFIG_AP_VHT;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_spi2_get(void)
{
#ifdef DD_DEV_TYPE_SPI2
	return DD_DEV_TYPE_SPI2;
#else
	return 0;
#endif
}

uint32_t lib_config_ftpd_upgrade_get(void)
{
#ifdef CONFIG_FTPD_UPGRADE
	return CONFIG_FTPD_UPGRADE;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_cal_get(void)
{
#ifdef DD_DEV_TYPE_CAL
	return DD_DEV_TYPE_CAL;
#else
	return 0;
#endif
}

uint32_t lib_config_bt_mesh_health_cli_get(void)
{
#ifdef CONFIG_BT_MESH_HEALTH_CLI
	return CONFIG_BT_MESH_HEALTH_CLI;
#else
	return 0;
#endif
}

uint32_t lib_config_dsp_get(void)
{
#ifdef CONFIG_DSP
	return CONFIG_DSP;
#else
	return 0;
#endif
}

uint32_t lib_config_sdio_get(void)
{
#ifdef CONFIG_SDIO
	return CONFIG_SDIO;
#else
	return 0;
#endif
}

uint32_t lib_config_mcu_ps_get(void)
{
#ifdef CONFIG_MCU_PS
	return CONFIG_MCU_PS;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_fast_connect_get(void)
{
#ifdef CONFIG_WIFI_FAST_CONNECT
	return CONFIG_WIFI_FAST_CONNECT;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_start_get(void)
{
#ifdef DD_DEV_TYPE_START
	return DD_DEV_TYPE_START;
#else
	return 0;
#endif
}

uint32_t lib_config_tcp_server_test_get(void)
{
#ifdef CONFIG_TCP_SERVER_TEST
	return CONFIG_TCP_SERVER_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_sys_reduce_normal_power_get(void)
{
#ifdef CONFIG_SYS_REDUCE_NORMAL_POWER
	return CONFIG_SYS_REDUCE_NORMAL_POWER;
#else
	return 0;
#endif
}

uint32_t lib_config_qspi_get(void)
{
#ifdef CONFIG_QSPI
	return CONFIG_QSPI;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_sdcard_get(void)
{
#ifdef DD_DEV_TYPE_SDCARD
	return DD_DEV_TYPE_SDCARD;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_none_get(void)
{
#ifdef DD_DEV_TYPE_NONE
	return DD_DEV_TYPE_NONE;
#else
	return 0;
#endif
}

uint32_t lib_config_sdio_trans_get(void)
{
#ifdef CONFIG_SDIO_TRANS
	return CONFIG_SDIO_TRANS;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_gpio_get(void)
{
#ifdef DD_DEV_TYPE_GPIO
	return DD_DEV_TYPE_GPIO;
#else
	return 0;
#endif
}

uint32_t lib_config_rx_sense_test_get(void)
{
#ifdef CONFIG_RX_SENSE_TEST
	return CONFIG_RX_SENSE_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_adv_num_get(void)
{
#ifdef CONFIG_BLE_ADV_NUM
	return CONFIG_BLE_ADV_NUM;
#else
	return 0;
#endif
}

uint32_t lib_config_lwip_mem_default_get(void)
{
#ifdef CONFIG_LWIP_MEM_DEFAULT
	return CONFIG_LWIP_MEM_DEFAULT;
#else
	return 0;
#endif
}

uint32_t lib_config_power_table_get(void)
{
#ifdef CONFIG_POWER_TABLE
	return CONFIG_POWER_TABLE;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_wdt_get(void)
{
#ifdef DD_DEV_TYPE_WDT
	return DD_DEV_TYPE_WDT;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_i2c1_get(void)
{
#ifdef DD_DEV_TYPE_I2C1
	return DD_DEV_TYPE_I2C1;
#else
	return 0;
#endif
}

uint32_t lib_config_bk_assert_halt_get(void)
{
#ifdef CONFIG_BK_ASSERT_HALT
	return CONFIG_BK_ASSERT_HALT;
#else
	return 0;
#endif
}

uint32_t lib_config_saradc_cali_get(void)
{
#ifdef CONFIG_SARADC_CALI
	return CONFIG_SARADC_CALI;
#else
	return 0;
#endif
}

uint32_t lib_config_task_reconnect_prio_get(void)
{
#ifdef CONFIG_TASK_RECONNECT_PRIO
	return CONFIG_TASK_RECONNECT_PRIO;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_scan_ch_time_get(void)
{
#ifdef CONFIG_WIFI_SCAN_CH_TIME
	return CONFIG_WIFI_SCAN_CH_TIME;
#else
	return 0;
#endif
}

uint32_t lib_config_i2c1_test_get(void)
{
#ifdef CONFIG_I2C1_TEST
	return CONFIG_I2C1_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_freertos_v10_get(void)
{
#ifdef CONFIG_FREERTOS_V10
	return CONFIG_FREERTOS_V10;
#else
	return 0;
#endif
}

uint32_t lib_config_jtag_get(void)
{
#ifdef CONFIG_JTAG
	return CONFIG_JTAG;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_spi3_get(void)
{
#ifdef DD_DEV_TYPE_SPI3
	return DD_DEV_TYPE_SPI3;
#else
	return 0;
#endif
}

uint32_t lib_config_base_mac_from_efuse_get(void)
{
#ifdef CONFIG_BASE_MAC_FROM_EFUSE
	return CONFIG_BASE_MAC_FROM_EFUSE;
#else
	return 0;
#endif
}

uint32_t lib_config_real_sdio_get(void)
{
#ifdef CONFIG_REAL_SDIO
	return CONFIG_REAL_SDIO;
#else
	return 0;
#endif
}

uint32_t lib_config_sdio_block_512_get(void)
{
#ifdef CONFIG_SDIO_BLOCK_512
	return CONFIG_SDIO_BLOCK_512;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_host_zephyr_get(void)
{
#ifdef CONFIG_BLE_HOST_ZEPHYR
	return CONFIG_BLE_HOST_ZEPHYR;
#else
	return 0;
#endif
}

uint32_t lib_config_soc_bk7251_get(void)
{
#ifdef CONFIG_SOC_BK7251
	return CONFIG_SOC_BK7251;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi4_get(void)
{
#ifdef CONFIG_WIFI4
	return CONFIG_WIFI4;
#else
	return 0;
#endif
}

uint32_t lib_config_soc_bk7231_get(void)
{
#ifdef CONFIG_SOC_BK7231
	return CONFIG_SOC_BK7231;
#else
	return 0;
#endif
}

uint32_t lib_config_usb_charge_get(void)
{
#ifdef CONFIG_USB_CHARGE
	return CONFIG_USB_CHARGE;
#else
	return 0;
#endif
}

uint32_t lib_config_ap_ps_get(void)
{
#ifdef CONFIG_AP_PS
	return CONFIG_AP_PS;
#else
	return 0;
#endif
}

uint32_t lib_config_less_memery_in_rwnx_get(void)
{
#ifdef CONFIG_LESS_MEMERY_IN_RWNX
	return CONFIG_LESS_MEMERY_IN_RWNX;
#else
	return 0;
#endif
}

uint32_t lib_config_uart_dma_get(void)
{
#ifdef CONFIG_UART_DMA
	return CONFIG_UART_DMA;
#else
	return 0;
#endif
}

uint32_t lib_config_bk7221_mdm_watchdog_patch_get(void)
{
#ifdef CONFIG_BK7221_MDM_WATCHDOG_PATCH
	return CONFIG_BK7221_MDM_WATCHDOG_PATCH;
#else
	return 0;
#endif
}

uint32_t lib_config_spidma_get(void)
{
#ifdef CONFIG_SPIDMA
	return CONFIG_SPIDMA;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_4_2_get(void)
{
#ifdef CONFIG_BLE_4_2
	return CONFIG_BLE_4_2;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_init_num_get(void)
{
#ifdef CONFIG_BLE_INIT_NUM
	return CONFIG_BLE_INIT_NUM;
#else
	return 0;
#endif
}

uint32_t lib_config_support_irda_get(void)
{
#ifdef CONFIG_SUPPORT_IRDA
	return CONFIG_SUPPORT_IRDA;
#else
	return 0;
#endif
}

uint32_t lib_config_app_demo_video_transfer_get(void)
{
#ifdef CONFIG_APP_DEMO_VIDEO_TRANSFER
	return CONFIG_APP_DEMO_VIDEO_TRANSFER;
#else
	return 0;
#endif
}

uint32_t lib_config_tx_evm_test_get(void)
{
#ifdef CONFIG_TX_EVM_TEST
	return CONFIG_TX_EVM_TEST;
#else
	return 0;
#endif
}

uint32_t lib_config_ap_probereq_cb_get(void)
{
#ifdef CONFIG_AP_PROBEREQ_CB
	return CONFIG_AP_PROBEREQ_CB;
#else
	return 0;
#endif
}

uint32_t lib_config_pcm_resampler_get(void)
{
#ifdef CONFIG_PCM_RESAMPLER
	return CONFIG_PCM_RESAMPLER;
#else
	return 0;
#endif
}

uint32_t lib_config_button_get(void)
{
#ifdef CONFIG_BUTTON
	return CONFIG_BUTTON;
#else
	return 0;
#endif
}

uint32_t lib_dd_dev_type_i2c2_get(void)
{
#ifdef DD_DEV_TYPE_I2C2
	return DD_DEV_TYPE_I2C2;
#else
	return 0;
#endif
}

uint32_t lib_config_ble_get(void)
{
#ifdef CONFIG_BLE
	return CONFIG_BLE;
#else
	return 0;
#endif
}

uint32_t lib_config_wifi_wps_get(void)
{
#ifdef CONFIG_WIFI_WPS
	return CONFIG_WIFI_WPS;
#else
	return 0;
#endif
}
