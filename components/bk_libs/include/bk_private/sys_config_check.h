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


#pragma once

#include "sys_config.h"

#include "drv_model_pub.h"

bool bk_config_check(void);

uint32_t config_sdio_credits_get(void);
uint32_t lib_config_sdio_credits_get(void);
#define    CONFIG_SDIO_CREDITS_CHECK()\
	if (config_sdio_credits_get() != lib_config_sdio_credits_get()) {\
		BK_LOGW(TAG, "CONFIG_SDIO_CREDITS mismatch(%x vs %x)\n", config_sdio_credits_get(), lib_config_sdio_credits_get());\
	}

uint32_t config_camera_get(void);
uint32_t lib_config_camera_get(void);
#define    CONFIG_CAMERA_CHECK()\
	if (config_camera_get() != lib_config_camera_get()) {\
		BK_LOGW(TAG, "CONFIG_CAMERA mismatch(%x vs %x)\n", config_camera_get(), lib_config_camera_get());\
	}

uint32_t config_spi_master_get(void);
uint32_t lib_config_spi_master_get(void);
#define    CONFIG_SPI_MASTER_CHECK()\
	if (config_spi_master_get() != lib_config_spi_master_get()) {\
		BK_LOGW(TAG, "CONFIG_SPI_MASTER mismatch(%x vs %x)\n", config_spi_master_get(), lib_config_spi_master_get());\
	}

uint32_t config_beacon_vendor_api_get(void);
uint32_t lib_config_beacon_vendor_api_get(void);
#define    CONFIG_BEACON_VENDOR_API_CHECK()\
	if (config_beacon_vendor_api_get() != lib_config_beacon_vendor_api_get()) {\
		BK_LOGW(TAG, "CONFIG_BEACON_VENDOR_API mismatch(%x vs %x)\n", config_beacon_vendor_api_get(), lib_config_beacon_vendor_api_get());\
	}

uint32_t dd_dev_type_saradc_get(void);
uint32_t lib_dd_dev_type_saradc_get(void);
#define    DD_DEV_TYPE_SARADC_CHECK()\
	if (dd_dev_type_saradc_get() != lib_dd_dev_type_saradc_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SARADC mismatch(%x vs %x)\n", dd_dev_type_saradc_get(), lib_dd_dev_type_saradc_get());\
	}

uint32_t config_ble_ps_get(void);
uint32_t lib_config_ble_ps_get(void);
#define    CONFIG_BLE_PS_CHECK()\
	if (config_ble_ps_get() != lib_config_ble_ps_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_PS mismatch(%x vs %x)\n", config_ble_ps_get(), lib_config_ble_ps_get());\
	}

uint32_t dd_dev_type_uart2_get(void);
uint32_t lib_dd_dev_type_uart2_get(void);
#define    DD_DEV_TYPE_UART2_CHECK()\
	if (dd_dev_type_uart2_get() != lib_dd_dev_type_uart2_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_UART2 mismatch(%x vs %x)\n", dd_dev_type_uart2_get(), lib_dd_dev_type_uart2_get());\
	}

uint32_t config_wpa3_enterprise_get(void);
uint32_t lib_config_wpa3_enterprise_get(void);
#define    CONFIG_WPA3_ENTERPRISE_CHECK()\
	if (config_wpa3_enterprise_get() != lib_config_wpa3_enterprise_get()) {\
		BK_LOGW(TAG, "CONFIG_WPA3_ENTERPRISE mismatch(%x vs %x)\n", config_wpa3_enterprise_get(), lib_config_wpa3_enterprise_get());\
	}

uint32_t config_temp_detect_get(void);
uint32_t lib_config_temp_detect_get(void);
#define    CONFIG_TEMP_DETECT_CHECK()\
	if (config_temp_detect_get() != lib_config_temp_detect_get()) {\
		BK_LOGW(TAG, "CONFIG_TEMP_DETECT mismatch(%x vs %x)\n", config_temp_detect_get(), lib_config_temp_detect_get());\
	}

uint32_t config_mac_phy_bypass_get(void);
uint32_t lib_config_mac_phy_bypass_get(void);
#define    CONFIG_MAC_PHY_BYPASS_CHECK()\
	if (config_mac_phy_bypass_get() != lib_config_mac_phy_bypass_get()) {\
		BK_LOGW(TAG, "CONFIG_MAC_PHY_BYPASS mismatch(%x vs %x)\n", config_mac_phy_bypass_get(), lib_config_mac_phy_bypass_get());\
	}

uint32_t dd_dev_type_aud_dac_get(void);
uint32_t lib_dd_dev_type_aud_dac_get(void);
#define    DD_DEV_TYPE_AUD_DAC_CHECK()\
	if (dd_dev_type_aud_dac_get() != lib_dd_dev_type_aud_dac_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_AUD_DAC mismatch(%x vs %x)\n", dd_dev_type_aud_dac_get(), lib_dd_dev_type_aud_dac_get());\
	}

uint32_t config_soc_bk7236_get(void);
uint32_t lib_config_soc_bk7236_get(void);
#define    CONFIG_SOC_BK7236_CHECK()\
	if (config_soc_bk7236_get() != lib_config_soc_bk7236_get()) {\
		BK_LOGW(TAG, "CONFIG_SOC_BK7236 mismatch(%x vs %x)\n", config_soc_bk7236_get(), lib_config_soc_bk7236_get());\
	}

uint32_t config_wifi6_ip_debug_get(void);
uint32_t lib_config_wifi6_ip_debug_get(void);
#define    CONFIG_WIFI6_IP_DEBUG_CHECK()\
	if (config_wifi6_ip_debug_get() != lib_config_wifi6_ip_debug_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI6_IP_DEBUG mismatch(%x vs %x)\n", config_wifi6_ip_debug_get(), lib_config_wifi6_ip_debug_get());\
	}

uint32_t dd_dev_type_dsp_get(void);
uint32_t lib_dd_dev_type_dsp_get(void);
#define    DD_DEV_TYPE_DSP_CHECK()\
	if (dd_dev_type_dsp_get() != lib_dd_dev_type_dsp_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_DSP mismatch(%x vs %x)\n", dd_dev_type_dsp_get(), lib_dd_dev_type_dsp_get());\
	}

uint32_t config_wifi_p2p_go_get(void);
uint32_t lib_config_wifi_p2p_go_get(void);
#define    CONFIG_WIFI_P2P_GO_CHECK()\
	if (config_wifi_p2p_go_get() != lib_config_wifi_p2p_go_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_P2P_GO mismatch(%x vs %x)\n", config_wifi_p2p_go_get(), lib_config_wifi_p2p_go_get());\
	}

uint32_t config_ble_mesh_rw_get(void);
uint32_t lib_config_ble_mesh_rw_get(void);
#define    CONFIG_BLE_MESH_RW_CHECK()\
	if (config_ble_mesh_rw_get() != lib_config_ble_mesh_rw_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_MESH_RW mismatch(%x vs %x)\n", config_ble_mesh_rw_get(), lib_config_ble_mesh_rw_get());\
	}

uint32_t dd_dev_type_mpb_get(void);
uint32_t lib_dd_dev_type_mpb_get(void);
#define    DD_DEV_TYPE_MPB_CHECK()\
	if (dd_dev_type_mpb_get() != lib_dd_dev_type_mpb_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_MPB mismatch(%x vs %x)\n", dd_dev_type_mpb_get(), lib_dd_dev_type_mpb_get());\
	}

uint32_t config_dhcp_get(void);
uint32_t lib_config_dhcp_get(void);
#define    CONFIG_DHCP_CHECK()\
	if (config_dhcp_get() != lib_config_dhcp_get()) {\
		BK_LOGW(TAG, "CONFIG_DHCP mismatch(%x vs %x)\n", config_dhcp_get(), lib_config_dhcp_get());\
	}

uint32_t config_cali_get(void);
uint32_t lib_config_cali_get(void);
#define    CONFIG_CALI_CHECK()\
	if (config_cali_get() != lib_config_cali_get()) {\
		BK_LOGW(TAG, "CONFIG_CALI mismatch(%x vs %x)\n", config_cali_get(), lib_config_cali_get());\
	}

uint32_t dd_dev_type_sctrl_get(void);
uint32_t lib_dd_dev_type_sctrl_get(void);
#define    DD_DEV_TYPE_SCTRL_CHECK()\
	if (dd_dev_type_sctrl_get() != lib_dd_dev_type_sctrl_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SCTRL mismatch(%x vs %x)\n", dd_dev_type_sctrl_get(), lib_dd_dev_type_sctrl_get());\
	}

uint32_t config_rf_ota_test_get(void);
uint32_t lib_config_rf_ota_test_get(void);
#define    CONFIG_RF_OTA_TEST_CHECK()\
	if (config_rf_ota_test_get() != lib_config_rf_ota_test_get()) {\
		BK_LOGW(TAG, "CONFIG_RF_OTA_TEST mismatch(%x vs %x)\n", config_rf_ota_test_get(), lib_config_rf_ota_test_get());\
	}

uint32_t config_background_print_get(void);
uint32_t lib_config_background_print_get(void);
#define    CONFIG_BACKGROUND_PRINT_CHECK()\
	if (config_background_print_get() != lib_config_background_print_get()) {\
		BK_LOGW(TAG, "CONFIG_BACKGROUND_PRINT mismatch(%x vs %x)\n", config_background_print_get(), lib_config_background_print_get());\
	}

uint32_t config_task_usb_prio_get(void);
uint32_t lib_config_task_usb_prio_get(void);
#define    CONFIG_TASK_USB_PRIO_CHECK()\
	if (config_task_usb_prio_get() != lib_config_task_usb_prio_get()) {\
		BK_LOGW(TAG, "CONFIG_TASK_USB_PRIO mismatch(%x vs %x)\n", config_task_usb_prio_get(), lib_config_task_usb_prio_get());\
	}

uint32_t config_uart_debug_get(void);
uint32_t lib_config_uart_debug_get(void);
#define    CONFIG_UART_DEBUG_CHECK()\
	if (config_uart_debug_get() != lib_config_uart_debug_get()) {\
		BK_LOGW(TAG, "CONFIG_UART_DEBUG mismatch(%x vs %x)\n", config_uart_debug_get(), lib_config_uart_debug_get());\
	}

uint32_t config_fpga_get(void);
uint32_t lib_config_fpga_get(void);
#define    CONFIG_FPGA_CHECK()\
	if (config_fpga_get() != lib_config_fpga_get()) {\
		BK_LOGW(TAG, "CONFIG_FPGA mismatch(%x vs %x)\n", config_fpga_get(), lib_config_fpga_get());\
	}

uint32_t config_rf_policy_ble_req_get(void);
uint32_t lib_config_rf_policy_ble_req_get(void);
#define    CONFIG_RF_POLICY_BLE_REQ_CHECK()\
	if (config_rf_policy_ble_req_get() != lib_config_rf_policy_ble_req_get()) {\
		BK_LOGW(TAG, "CONFIG_RF_POLICY_BLE_REQ mismatch(%x vs %x)\n", config_rf_policy_ble_req_get(), lib_config_rf_policy_ble_req_get());\
	}

uint32_t config_hslave_spi_get(void);
uint32_t lib_config_hslave_spi_get(void);
#define    CONFIG_HSLAVE_SPI_CHECK()\
	if (config_hslave_spi_get() != lib_config_hslave_spi_get()) {\
		BK_LOGW(TAG, "CONFIG_HSLAVE_SPI mismatch(%x vs %x)\n", config_hslave_spi_get(), lib_config_hslave_spi_get());\
	}

uint32_t config_usb_host_get(void);
uint32_t lib_config_usb_host_get(void);
#define    CONFIG_USB_HOST_CHECK()\
	if (config_usb_host_get() != lib_config_usb_host_get()) {\
		BK_LOGW(TAG, "CONFIG_USB_HOST mismatch(%x vs %x)\n", config_usb_host_get(), lib_config_usb_host_get());\
	}

uint32_t config_msdu_resv_head_len_get(void);
uint32_t lib_config_msdu_resv_head_len_get(void);
#define    CONFIG_MSDU_RESV_HEAD_LEN_CHECK()\
	if (config_msdu_resv_head_len_get() != lib_config_msdu_resv_head_len_get()) {\
		BK_LOGW(TAG, "CONFIG_MSDU_RESV_HEAD_LEN mismatch(%x vs %x)\n", config_msdu_resv_head_len_get(), lib_config_msdu_resv_head_len_get());\
	}

uint32_t dd_dev_type_sdio_get(void);
uint32_t lib_dd_dev_type_sdio_get(void);
#define    DD_DEV_TYPE_SDIO_CHECK()\
	if (dd_dev_type_sdio_get() != lib_dd_dev_type_sdio_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SDIO mismatch(%x vs %x)\n", dd_dev_type_sdio_get(), lib_dd_dev_type_sdio_get());\
	}

uint32_t config_int_wdt_period_ms_get(void);
uint32_t lib_config_int_wdt_period_ms_get(void);
#define    CONFIG_INT_WDT_PERIOD_MS_CHECK()\
	if (config_int_wdt_period_ms_get() != lib_config_int_wdt_period_ms_get()) {\
		BK_LOGW(TAG, "CONFIG_INT_WDT_PERIOD_MS mismatch(%x vs %x)\n", config_int_wdt_period_ms_get(), lib_config_int_wdt_period_ms_get());\
	}

uint32_t config_malloc_statis_get(void);
uint32_t lib_config_malloc_statis_get(void);
#define    CONFIG_MALLOC_STATIS_CHECK()\
	if (config_malloc_statis_get() != lib_config_malloc_statis_get()) {\
		BK_LOGW(TAG, "CONFIG_MALLOC_STATIS mismatch(%x vs %x)\n", config_malloc_statis_get(), lib_config_malloc_statis_get());\
	}

uint32_t config_bt_src_add_get(void);
uint32_t lib_config_bt_src_add_get(void);
#define    CONFIG_BT_SRC_ADD_CHECK()\
	if (config_bt_src_add_get() != lib_config_bt_src_add_get()) {\
		BK_LOGW(TAG, "CONFIG_BT_SRC_ADD mismatch(%x vs %x)\n", config_bt_src_add_get(), lib_config_bt_src_add_get());\
	}

uint32_t config_uart3_get(void);
uint32_t lib_config_uart3_get(void);
#define    CONFIG_UART3_CHECK()\
	if (config_uart3_get() != lib_config_uart3_get()) {\
		BK_LOGW(TAG, "CONFIG_UART3 mismatch(%x vs %x)\n", config_uart3_get(), lib_config_uart3_get());\
	}

uint32_t config_wifi_raw_tx_test_get(void);
uint32_t lib_config_wifi_raw_tx_test_get(void);
#define    CONFIG_WIFI_RAW_TX_TEST_CHECK()\
	if (config_wifi_raw_tx_test_get() != lib_config_wifi_raw_tx_test_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_RAW_TX_TEST mismatch(%x vs %x)\n", config_wifi_raw_tx_test_get(), lib_config_wifi_raw_tx_test_get());\
	}

uint32_t dd_dev_type_irda_get(void);
uint32_t lib_dd_dev_type_irda_get(void);
#define    DD_DEV_TYPE_IRDA_CHECK()\
	if (dd_dev_type_irda_get() != lib_dd_dev_type_irda_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_IRDA mismatch(%x vs %x)\n", dd_dev_type_irda_get(), lib_dd_dev_type_irda_get());\
	}

uint32_t config_peri_test_get(void);
uint32_t lib_config_peri_test_get(void);
#define    CONFIG_PERI_TEST_CHECK()\
	if (config_peri_test_get() != lib_config_peri_test_get()) {\
		BK_LOGW(TAG, "CONFIG_PERI_TEST mismatch(%x vs %x)\n", config_peri_test_get(), lib_config_peri_test_get());\
	}

uint32_t config_wpa_ctrl_iface_get(void);
uint32_t lib_config_wpa_ctrl_iface_get(void);
#define    CONFIG_WPA_CTRL_IFACE_CHECK()\
	if (config_wpa_ctrl_iface_get() != lib_config_wpa_ctrl_iface_get()) {\
		BK_LOGW(TAG, "CONFIG_WPA_CTRL_IFACE mismatch(%x vs %x)\n", config_wpa_ctrl_iface_get(), lib_config_wpa_ctrl_iface_get());\
	}

uint32_t dd_dev_type_usb_plug_get(void);
uint32_t lib_dd_dev_type_usb_plug_get(void);
#define    DD_DEV_TYPE_USB_PLUG_CHECK()\
	if (dd_dev_type_usb_plug_get() != lib_dd_dev_type_usb_plug_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_USB_PLUG mismatch(%x vs %x)\n", dd_dev_type_usb_plug_get(), lib_dd_dev_type_usb_plug_get());\
	}

uint32_t config_fake_rtc_ps_get(void);
uint32_t lib_config_fake_rtc_ps_get(void);
#define    CONFIG_FAKE_RTC_PS_CHECK()\
	if (config_fake_rtc_ps_get() != lib_config_fake_rtc_ps_get()) {\
		BK_LOGW(TAG, "CONFIG_FAKE_RTC_PS mismatch(%x vs %x)\n", config_fake_rtc_ps_get(), lib_config_fake_rtc_ps_get());\
	}

uint32_t config_rwnx_qos_msdu_get(void);
uint32_t lib_config_rwnx_qos_msdu_get(void);
#define    CONFIG_RWNX_QOS_MSDU_CHECK()\
	if (config_rwnx_qos_msdu_get() != lib_config_rwnx_qos_msdu_get()) {\
		BK_LOGW(TAG, "CONFIG_RWNX_QOS_MSDU mismatch(%x vs %x)\n", config_rwnx_qos_msdu_get(), lib_config_rwnx_qos_msdu_get());\
	}

uint32_t config_spi_slave_get(void);
uint32_t lib_config_spi_slave_get(void);
#define    CONFIG_SPI_SLAVE_CHECK()\
	if (config_spi_slave_get() != lib_config_spi_slave_get()) {\
		BK_LOGW(TAG, "CONFIG_SPI_SLAVE mismatch(%x vs %x)\n", config_spi_slave_get(), lib_config_spi_slave_get());\
	}

uint32_t config_soc_bk7271_get(void);
uint32_t lib_config_soc_bk7271_get(void);
#define    CONFIG_SOC_BK7271_CHECK()\
	if (config_soc_bk7271_get() != lib_config_soc_bk7271_get()) {\
		BK_LOGW(TAG, "CONFIG_SOC_BK7271 mismatch(%x vs %x)\n", config_soc_bk7271_get(), lib_config_soc_bk7271_get());\
	}

uint32_t config_fft_get(void);
uint32_t lib_config_fft_get(void);
#define    CONFIG_FFT_CHECK()\
	if (config_fft_get() != lib_config_fft_get()) {\
		BK_LOGW(TAG, "CONFIG_FFT mismatch(%x vs %x)\n", config_fft_get(), lib_config_fft_get());\
	}

uint32_t config_usb_uvc_get(void);
uint32_t lib_config_usb_uvc_get(void);
#define    CONFIG_USB_UVC_CHECK()\
	if (config_usb_uvc_get() != lib_config_usb_uvc_get()) {\
		BK_LOGW(TAG, "CONFIG_USB_UVC mismatch(%x vs %x)\n", config_usb_uvc_get(), lib_config_usb_uvc_get());\
	}

uint32_t config_no_hosted_get(void);
uint32_t lib_config_no_hosted_get(void);
#define    CONFIG_NO_HOSTED_CHECK()\
	if (config_no_hosted_get() != lib_config_no_hosted_get()) {\
		BK_LOGW(TAG, "CONFIG_NO_HOSTED mismatch(%x vs %x)\n", config_no_hosted_get(), lib_config_no_hosted_get());\
	}

uint32_t config_uart_test_get(void);
uint32_t lib_config_uart_test_get(void);
#define    CONFIG_UART_TEST_CHECK()\
	if (config_uart_test_get() != lib_config_uart_test_get()) {\
		BK_LOGW(TAG, "CONFIG_UART_TEST mismatch(%x vs %x)\n", config_uart_test_get(), lib_config_uart_test_get());\
	}

uint32_t config_mp3player_get(void);
uint32_t lib_config_mp3player_get(void);
#define    CONFIG_MP3PLAYER_CHECK()\
	if (config_mp3player_get() != lib_config_mp3player_get()) {\
		BK_LOGW(TAG, "CONFIG_MP3PLAYER mismatch(%x vs %x)\n", config_mp3player_get(), lib_config_mp3player_get());\
	}

uint32_t dd_dev_type_uart3_get(void);
uint32_t lib_dd_dev_type_uart3_get(void);
#define    DD_DEV_TYPE_UART3_CHECK()\
	if (dd_dev_type_uart3_get() != lib_dd_dev_type_uart3_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_UART3 mismatch(%x vs %x)\n", dd_dev_type_uart3_get(), lib_dd_dev_type_uart3_get());\
	}

uint32_t config_bssid_connect_get(void);
uint32_t lib_config_bssid_connect_get(void);
#define    CONFIG_BSSID_CONNECT_CHECK()\
	if (config_bssid_connect_get() != lib_config_bssid_connect_get()) {\
		BK_LOGW(TAG, "CONFIG_BSSID_CONNECT mismatch(%x vs %x)\n", config_bssid_connect_get(), lib_config_bssid_connect_get());\
	}

uint32_t config_bt_mesh_cfg_cli_get(void);
uint32_t lib_config_bt_mesh_cfg_cli_get(void);
#define    CONFIG_BT_MESH_CFG_CLI_CHECK()\
	if (config_bt_mesh_cfg_cli_get() != lib_config_bt_mesh_cfg_cli_get()) {\
		BK_LOGW(TAG, "CONFIG_BT_MESH_CFG_CLI mismatch(%x vs %x)\n", config_bt_mesh_cfg_cli_get(), lib_config_bt_mesh_cfg_cli_get());\
	}

uint32_t config_wpa_log_get(void);
uint32_t lib_config_wpa_log_get(void);
#define    CONFIG_WPA_LOG_CHECK()\
	if (config_wpa_log_get() != lib_config_wpa_log_get()) {\
		BK_LOGW(TAG, "CONFIG_WPA_LOG mismatch(%x vs %x)\n", config_wpa_log_get(), lib_config_wpa_log_get());\
	}

uint32_t config_lwip_get(void);
uint32_t lib_config_lwip_get(void);
#define    CONFIG_LWIP_CHECK()\
	if (config_lwip_get() != lib_config_lwip_get()) {\
		BK_LOGW(TAG, "CONFIG_LWIP mismatch(%x vs %x)\n", config_lwip_get(), lib_config_lwip_get());\
	}

uint32_t config_manual_cali_get(void);
uint32_t lib_config_manual_cali_get(void);
#define    CONFIG_MANUAL_CALI_CHECK()\
	if (config_manual_cali_get() != lib_config_manual_cali_get()) {\
		BK_LOGW(TAG, "CONFIG_MANUAL_CALI mismatch(%x vs %x)\n", config_manual_cali_get(), lib_config_manual_cali_get());\
	}

uint32_t config_task_wpas_prio_get(void);
uint32_t lib_config_task_wpas_prio_get(void);
#define    CONFIG_TASK_WPAS_PRIO_CHECK()\
	if (config_task_wpas_prio_get() != lib_config_task_wpas_prio_get()) {\
		BK_LOGW(TAG, "CONFIG_TASK_WPAS_PRIO mismatch(%x vs %x)\n", config_task_wpas_prio_get(), lib_config_task_wpas_prio_get());\
	}

uint32_t config_wifi6_code_stack_get(void);
uint32_t lib_config_wifi6_code_stack_get(void);
#define    CONFIG_WIFI6_CODE_STACK_CHECK()\
	if (config_wifi6_code_stack_get() != lib_config_wifi6_code_stack_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI6_CODE_STACK mismatch(%x vs %x)\n", config_wifi6_code_stack_get(), lib_config_wifi6_code_stack_get());\
	}

uint32_t config_sdcard_host_get(void);
uint32_t lib_config_sdcard_host_get(void);
#define    CONFIG_SDCARD_HOST_CHECK()\
	if (config_sdcard_host_get() != lib_config_sdcard_host_get()) {\
		BK_LOGW(TAG, "CONFIG_SDCARD_HOST mismatch(%x vs %x)\n", config_sdcard_host_get(), lib_config_sdcard_host_get());\
	}

uint32_t config_soc_bk7231n_get(void);
uint32_t lib_config_soc_bk7231n_get(void);
#define    CONFIG_SOC_BK7231N_CHECK()\
	if (config_soc_bk7231n_get() != lib_config_soc_bk7231n_get()) {\
		BK_LOGW(TAG, "CONFIG_SOC_BK7231N mismatch(%x vs %x)\n", config_soc_bk7231n_get(), lib_config_soc_bk7231n_get());\
	}

uint32_t config_usb_device_get(void);
uint32_t lib_config_usb_device_get(void);
#define    CONFIG_USB_DEVICE_CHECK()\
	if (config_usb_device_get() != lib_config_usb_device_get()) {\
		BK_LOGW(TAG, "CONFIG_USB_DEVICE mismatch(%x vs %x)\n", config_usb_device_get(), lib_config_usb_device_get());\
	}

uint32_t config_udisk_mp3_get(void);
uint32_t lib_config_udisk_mp3_get(void);
#define    CONFIG_UDISK_MP3_CHECK()\
	if (config_udisk_mp3_get() != lib_config_udisk_mp3_get()) {\
		BK_LOGW(TAG, "CONFIG_UDISK_MP3 mismatch(%x vs %x)\n", config_udisk_mp3_get(), lib_config_udisk_mp3_get());\
	}

uint32_t config_use_conv_utf8_get(void);
uint32_t lib_config_use_conv_utf8_get(void);
#define    CONFIG_USE_CONV_UTF8_CHECK()\
	if (config_use_conv_utf8_get() != lib_config_use_conv_utf8_get()) {\
		BK_LOGW(TAG, "CONFIG_USE_CONV_UTF8 mismatch(%x vs %x)\n", config_use_conv_utf8_get(), lib_config_use_conv_utf8_get());\
	}

uint32_t config_bt_get(void);
uint32_t lib_config_bt_get(void);
#define    CONFIG_BT_CHECK()\
	if (config_bt_get() != lib_config_bt_get()) {\
		BK_LOGW(TAG, "CONFIG_BT mismatch(%x vs %x)\n", config_bt_get(), lib_config_bt_get());\
	}

uint32_t config_rf_policy_wifi_req_get(void);
uint32_t lib_config_rf_policy_wifi_req_get(void);
#define    CONFIG_RF_POLICY_WIFI_REQ_CHECK()\
	if (config_rf_policy_wifi_req_get() != lib_config_rf_policy_wifi_req_get()) {\
		BK_LOGW(TAG, "CONFIG_RF_POLICY_WIFI_REQ mismatch(%x vs %x)\n", config_rf_policy_wifi_req_get(), lib_config_rf_policy_wifi_req_get());\
	}

uint32_t dd_dev_type_fft_get(void);
uint32_t lib_dd_dev_type_fft_get(void);
#define    DD_DEV_TYPE_FFT_CHECK()\
	if (dd_dev_type_fft_get() != lib_dd_dev_type_fft_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_FFT mismatch(%x vs %x)\n", dd_dev_type_fft_get(), lib_dd_dev_type_fft_get());\
	}

uint32_t config_task_wdt_get(void);
uint32_t lib_config_task_wdt_get(void);
#define    CONFIG_TASK_WDT_CHECK()\
	if (config_task_wdt_get() != lib_config_task_wdt_get()) {\
		BK_LOGW(TAG, "CONFIG_TASK_WDT mismatch(%x vs %x)\n", config_task_wdt_get(), lib_config_task_wdt_get());\
	}

uint32_t config_uart1_get(void);
uint32_t lib_config_uart1_get(void);
#define    CONFIG_UART1_CHECK()\
	if (config_uart1_get() != lib_config_uart1_get()) {\
		BK_LOGW(TAG, "CONFIG_UART1 mismatch(%x vs %x)\n", config_uart1_get(), lib_config_uart1_get());\
	}

uint32_t config_customer_drone_get(void);
uint32_t lib_config_customer_drone_get(void);
#define    CONFIG_CUSTOMER_DRONE_CHECK()\
	if (config_customer_drone_get() != lib_config_customer_drone_get()) {\
		BK_LOGW(TAG, "CONFIG_CUSTOMER_DRONE mismatch(%x vs %x)\n", config_customer_drone_get(), lib_config_customer_drone_get());\
	}

uint32_t config_i2s_get(void);
uint32_t lib_config_i2s_get(void);
#define    CONFIG_I2S_CHECK()\
	if (config_i2s_get() != lib_config_i2s_get()) {\
		BK_LOGW(TAG, "CONFIG_I2S mismatch(%x vs %x)\n", config_i2s_get(), lib_config_i2s_get());\
	}

uint32_t config_bt_mesh_adv_legacy_get(void);
uint32_t lib_config_bt_mesh_adv_legacy_get(void);
#define    CONFIG_BT_MESH_ADV_LEGACY_CHECK()\
	if (config_bt_mesh_adv_legacy_get() != lib_config_bt_mesh_adv_legacy_get()) {\
		BK_LOGW(TAG, "CONFIG_BT_MESH_ADV_LEGACY mismatch(%x vs %x)\n", config_bt_mesh_adv_legacy_get(), lib_config_bt_mesh_adv_legacy_get());\
	}

uint32_t config_task_wdt_period_ms_get(void);
uint32_t lib_config_task_wdt_period_ms_get(void);
#define    CONFIG_TASK_WDT_PERIOD_MS_CHECK()\
	if (config_task_wdt_period_ms_get() != lib_config_task_wdt_period_ms_get()) {\
		BK_LOGW(TAG, "CONFIG_TASK_WDT_PERIOD_MS mismatch(%x vs %x)\n", config_task_wdt_period_ms_get(), lib_config_task_wdt_period_ms_get());\
	}

uint32_t config_spi_mst_flash_get(void);
uint32_t lib_config_spi_mst_flash_get(void);
#define    CONFIG_SPI_MST_FLASH_CHECK()\
	if (config_spi_mst_flash_get() != lib_config_spi_mst_flash_get()) {\
		BK_LOGW(TAG, "CONFIG_SPI_MST_FLASH mismatch(%x vs %x)\n", config_spi_mst_flash_get(), lib_config_spi_mst_flash_get());\
	}

uint32_t dd_dev_type_icu_get(void);
uint32_t lib_dd_dev_type_icu_get(void);
#define    DD_DEV_TYPE_ICU_CHECK()\
	if (dd_dev_type_icu_get() != lib_dd_dev_type_icu_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_ICU mismatch(%x vs %x)\n", dd_dev_type_icu_get(), lib_dd_dev_type_icu_get());\
	}

uint32_t config_ap_he_get(void);
uint32_t lib_config_ap_he_get(void);
#define    CONFIG_AP_HE_CHECK()\
	if (config_ap_he_get() != lib_config_ap_he_get()) {\
		BK_LOGW(TAG, "CONFIG_AP_HE mismatch(%x vs %x)\n", config_ap_he_get(), lib_config_ap_he_get());\
	}

uint32_t config_wifi6_get(void);
uint32_t lib_config_wifi6_get(void);
#define    CONFIG_WIFI6_CHECK()\
	if (config_wifi6_get() != lib_config_wifi6_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI6 mismatch(%x vs %x)\n", config_wifi6_get(), lib_config_wifi6_get());\
	}

uint32_t config_lwip_mem_reduce_get(void);
uint32_t lib_config_lwip_mem_reduce_get(void);
#define    CONFIG_LWIP_MEM_REDUCE_CHECK()\
	if (config_lwip_mem_reduce_get() != lib_config_lwip_mem_reduce_get()) {\
		BK_LOGW(TAG, "CONFIG_LWIP_MEM_REDUCE mismatch(%x vs %x)\n", config_lwip_mem_reduce_get(), lib_config_lwip_mem_reduce_get());\
	}

uint32_t config_wifi_kmsg_task_stack_size_get(void);
uint32_t lib_config_wifi_kmsg_task_stack_size_get(void);
#define    CONFIG_WIFI_KMSG_TASK_STACK_SIZE_CHECK()\
	if (config_wifi_kmsg_task_stack_size_get() != lib_config_wifi_kmsg_task_stack_size_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_KMSG_TASK_STACK_SIZE mismatch(%x vs %x)\n", config_wifi_kmsg_task_stack_size_get(), lib_config_wifi_kmsg_task_stack_size_get());\
	}

uint32_t config_usb_get(void);
uint32_t lib_config_usb_get(void);
#define    CONFIG_USB_CHECK()\
	if (config_usb_get() != lib_config_usb_get()) {\
		BK_LOGW(TAG, "CONFIG_USB mismatch(%x vs %x)\n", config_usb_get(), lib_config_usb_get());\
	}

uint32_t config_pmf_get(void);
uint32_t lib_config_pmf_get(void);
#define    CONFIG_PMF_CHECK()\
	if (config_pmf_get() != lib_config_pmf_get()) {\
		BK_LOGW(TAG, "CONFIG_PMF mismatch(%x vs %x)\n", config_pmf_get(), lib_config_pmf_get());\
	}

uint32_t config_tick_cali_get(void);
uint32_t lib_config_tick_cali_get(void);
#define    CONFIG_TICK_CALI_CHECK()\
	if (config_tick_cali_get() != lib_config_tick_cali_get()) {\
		BK_LOGW(TAG, "CONFIG_TICK_CALI mismatch(%x vs %x)\n", config_tick_cali_get(), lib_config_tick_cali_get());\
	}

uint32_t dd_dev_type_mailbox_get(void);
uint32_t lib_dd_dev_type_mailbox_get(void);
#define    DD_DEV_TYPE_MAILBOX_CHECK()\
	if (dd_dev_type_mailbox_get() != lib_dd_dev_type_mailbox_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_MAILBOX mismatch(%x vs %x)\n", dd_dev_type_mailbox_get(), lib_dd_dev_type_mailbox_get());\
	}

uint32_t config_usb1_port_get(void);
uint32_t lib_config_usb1_port_get(void);
#define    CONFIG_USB1_PORT_CHECK()\
	if (config_usb1_port_get() != lib_config_usb1_port_get()) {\
		BK_LOGW(TAG, "CONFIG_USB1_PORT mismatch(%x vs %x)\n", config_usb1_port_get(), lib_config_usb1_port_get());\
	}

uint32_t config_spi_test_get(void);
uint32_t lib_config_spi_test_get(void);
#define    CONFIG_SPI_TEST_CHECK()\
	if (config_spi_test_get() != lib_config_spi_test_get()) {\
		BK_LOGW(TAG, "CONFIG_SPI_TEST mismatch(%x vs %x)\n", config_spi_test_get(), lib_config_spi_test_get());\
	}

uint32_t config_ble_host_rw_get(void);
uint32_t lib_config_ble_host_rw_get(void);
#define    CONFIG_BLE_HOST_RW_CHECK()\
	if (config_ble_host_rw_get() != lib_config_ble_host_rw_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_HOST_RW mismatch(%x vs %x)\n", config_ble_host_rw_get(), lib_config_ble_host_rw_get());\
	}

uint32_t config_wifi_core_task_stack_size_get(void);
uint32_t lib_config_wifi_core_task_stack_size_get(void);
#define    CONFIG_WIFI_CORE_TASK_STACK_SIZE_CHECK()\
	if (config_wifi_core_task_stack_size_get() != lib_config_wifi_core_task_stack_size_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_CORE_TASK_STACK_SIZE mismatch(%x vs %x)\n", config_wifi_core_task_stack_size_get(), lib_config_wifi_core_task_stack_size_get());\
	}

uint32_t config_ble_conn_num_get(void);
uint32_t lib_config_ble_conn_num_get(void);
#define    CONFIG_BLE_CONN_NUM_CHECK()\
	if (config_ble_conn_num_get() != lib_config_ble_conn_num_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_CONN_NUM mismatch(%x vs %x)\n", config_ble_conn_num_get(), lib_config_ble_conn_num_get());\
	}

uint32_t config_uart2_get(void);
uint32_t lib_config_uart2_get(void);
#define    CONFIG_UART2_CHECK()\
	if (config_uart2_get() != lib_config_uart2_get()) {\
		BK_LOGW(TAG, "CONFIG_UART2 mismatch(%x vs %x)\n", config_uart2_get(), lib_config_uart2_get());\
	}

uint32_t config_wfa_cert_get(void);
uint32_t lib_config_wfa_cert_get(void);
#define    CONFIG_WFA_CERT_CHECK()\
	if (config_wfa_cert_get() != lib_config_wfa_cert_get()) {\
		BK_LOGW(TAG, "CONFIG_WFA_CERT mismatch(%x vs %x)\n", config_wfa_cert_get(), lib_config_wfa_cert_get());\
	}

uint32_t config_ble_5_x_get(void);
uint32_t lib_config_ble_5_x_get(void);
#define    CONFIG_BLE_5_X_CHECK()\
	if (config_ble_5_x_get() != lib_config_ble_5_x_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_5_X mismatch(%x vs %x)\n", config_ble_5_x_get(), lib_config_ble_5_x_get());\
	}

uint32_t config_trng_support_get(void);
uint32_t lib_config_trng_support_get(void);
#define    CONFIG_TRNG_SUPPORT_CHECK()\
	if (config_trng_support_get() != lib_config_trng_support_get()) {\
		BK_LOGW(TAG, "CONFIG_TRNG_SUPPORT mismatch(%x vs %x)\n", config_trng_support_get(), lib_config_trng_support_get());\
	}

uint32_t config_wpa_sme_get(void);
uint32_t lib_config_wpa_sme_get(void);
#define    CONFIG_WPA_SME_CHECK()\
	if (config_wpa_sme_get() != lib_config_wpa_sme_get()) {\
		BK_LOGW(TAG, "CONFIG_WPA_SME mismatch(%x vs %x)\n", config_wpa_sme_get(), lib_config_wpa_sme_get());\
	}

uint32_t config_at_get(void);
uint32_t lib_config_at_get(void);
#define    CONFIG_AT_CHECK()\
	if (config_at_get() != lib_config_at_get()) {\
		BK_LOGW(TAG, "CONFIG_AT mismatch(%x vs %x)\n", config_at_get(), lib_config_at_get());\
	}

uint32_t config_mem_debug_get(void);
uint32_t lib_config_mem_debug_get(void);
#define    CONFIG_MEM_DEBUG_CHECK()\
	if (config_mem_debug_get() != lib_config_mem_debug_get()) {\
		BK_LOGW(TAG, "CONFIG_MEM_DEBUG mismatch(%x vs %x)\n", config_mem_debug_get(), lib_config_mem_debug_get());\
	}

uint32_t config_ble_mesh_zephyr_get(void);
uint32_t lib_config_ble_mesh_zephyr_get(void);
#define    CONFIG_BLE_MESH_ZEPHYR_CHECK()\
	if (config_ble_mesh_zephyr_get() != lib_config_ble_mesh_zephyr_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_MESH_ZEPHYR mismatch(%x vs %x)\n", config_ble_mesh_zephyr_get(), lib_config_ble_mesh_zephyr_get());\
	}

uint32_t config_usb_ccd_get(void);
uint32_t lib_config_usb_ccd_get(void);
#define    CONFIG_USB_CCD_CHECK()\
	if (config_usb_ccd_get() != lib_config_usb_ccd_get()) {\
		BK_LOGW(TAG, "CONFIG_USB_CCD mismatch(%x vs %x)\n", config_usb_ccd_get(), lib_config_usb_ccd_get());\
	}

uint32_t config_ap_idle_get(void);
uint32_t lib_config_ap_idle_get(void);
#define    CONFIG_AP_IDLE_CHECK()\
	if (config_ap_idle_get() != lib_config_ap_idle_get()) {\
		BK_LOGW(TAG, "CONFIG_AP_IDLE mismatch(%x vs %x)\n", config_ap_idle_get(), lib_config_ap_idle_get());\
	}

uint32_t config_wifi_sensor_get(void);
uint32_t lib_config_wifi_sensor_get(void);
#define    CONFIG_WIFI_SENSOR_CHECK()\
	if (config_wifi_sensor_get() != lib_config_wifi_sensor_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_SENSOR mismatch(%x vs %x)\n", config_wifi_sensor_get(), lib_config_wifi_sensor_get());\
	}

uint32_t config_int_wdt_get(void);
uint32_t lib_config_int_wdt_get(void);
#define    CONFIG_INT_WDT_CHECK()\
	if (config_int_wdt_get() != lib_config_int_wdt_get()) {\
		BK_LOGW(TAG, "CONFIG_INT_WDT mismatch(%x vs %x)\n", config_int_wdt_get(), lib_config_int_wdt_get());\
	}

uint32_t config_beacon_update_api_get(void);
uint32_t lib_config_beacon_update_api_get(void);
#define    CONFIG_BEACON_UPDATE_API_CHECK()\
	if (config_beacon_update_api_get() != lib_config_beacon_update_api_get()) {\
		BK_LOGW(TAG, "CONFIG_BEACON_UPDATE_API mismatch(%x vs %x)\n", config_beacon_update_api_get(), lib_config_beacon_update_api_get());\
	}

uint32_t dd_dev_type_timer_get(void);
uint32_t lib_dd_dev_type_timer_get(void);
#define    DD_DEV_TYPE_TIMER_CHECK()\
	if (dd_dev_type_timer_get() != lib_dd_dev_type_timer_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_TIMER mismatch(%x vs %x)\n", dd_dev_type_timer_get(), lib_dd_dev_type_timer_get());\
	}

uint32_t dd_dev_type_ejpeg_get(void);
uint32_t lib_dd_dev_type_ejpeg_get(void);
#define    DD_DEV_TYPE_EJPEG_CHECK()\
	if (dd_dev_type_ejpeg_get() != lib_dd_dev_type_ejpeg_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_EJPEG mismatch(%x vs %x)\n", dd_dev_type_ejpeg_get(), lib_dd_dev_type_ejpeg_get());\
	}

uint32_t config_wifi_kmsg_task_prio_get(void);
uint32_t lib_config_wifi_kmsg_task_prio_get(void);
#define    CONFIG_WIFI_KMSG_TASK_PRIO_CHECK()\
	if (config_wifi_kmsg_task_prio_get() != lib_config_wifi_kmsg_task_prio_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_KMSG_TASK_PRIO mismatch(%x vs %x)\n", config_wifi_kmsg_task_prio_get(), lib_config_wifi_kmsg_task_prio_get());\
	}

uint32_t config_wifi_core_task_prio_get(void);
uint32_t lib_config_wifi_core_task_prio_get(void);
#define    CONFIG_WIFI_CORE_TASK_PRIO_CHECK()\
	if (config_wifi_core_task_prio_get() != lib_config_wifi_core_task_prio_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_CORE_TASK_PRIO mismatch(%x vs %x)\n", config_wifi_core_task_prio_get(), lib_config_wifi_core_task_prio_get());\
	}

uint32_t dd_dev_type_uart1_get(void);
uint32_t lib_dd_dev_type_uart1_get(void);
#define    DD_DEV_TYPE_UART1_CHECK()\
	if (dd_dev_type_uart1_get() != lib_dd_dev_type_uart1_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_UART1 mismatch(%x vs %x)\n", dd_dev_type_uart1_get(), lib_dd_dev_type_uart1_get());\
	}

uint32_t config_wpa2_enterprise_get(void);
uint32_t lib_config_wpa2_enterprise_get(void);
#define    CONFIG_WPA2_ENTERPRISE_CHECK()\
	if (config_wpa2_enterprise_get() != lib_config_wpa2_enterprise_get()) {\
		BK_LOGW(TAG, "CONFIG_WPA2_ENTERPRISE mismatch(%x vs %x)\n", config_wpa2_enterprise_get(), lib_config_wpa2_enterprise_get());\
	}

uint32_t config_general_dma_get(void);
uint32_t lib_config_general_dma_get(void);
#define    CONFIG_GENERAL_DMA_CHECK()\
	if (config_general_dma_get() != lib_config_general_dma_get()) {\
		BK_LOGW(TAG, "CONFIG_GENERAL_DMA mismatch(%x vs %x)\n", config_general_dma_get(), lib_config_general_dma_get());\
	}

uint32_t config_security_get(void);
uint32_t lib_config_security_get(void);
#define    CONFIG_SECURITY_CHECK()\
	if (config_security_get() != lib_config_security_get()) {\
		BK_LOGW(TAG, "CONFIG_SECURITY mismatch(%x vs %x)\n", config_security_get(), lib_config_security_get());\
	}

uint32_t dd_dev_type_qspi_get(void);
uint32_t lib_dd_dev_type_qspi_get(void);
#define    DD_DEV_TYPE_QSPI_CHECK()\
	if (dd_dev_type_qspi_get() != lib_dd_dev_type_qspi_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_QSPI mismatch(%x vs %x)\n", dd_dev_type_qspi_get(), lib_dd_dev_type_qspi_get());\
	}

uint32_t config_semi_hosted_get(void);
uint32_t lib_config_semi_hosted_get(void);
#define    CONFIG_SEMI_HOSTED_CHECK()\
	if (config_semi_hosted_get() != lib_config_semi_hosted_get()) {\
		BK_LOGW(TAG, "CONFIG_SEMI_HOSTED mismatch(%x vs %x)\n", config_semi_hosted_get(), lib_config_semi_hosted_get());\
	}

uint32_t dd_dev_type_usb_get(void);
uint32_t lib_dd_dev_type_usb_get(void);
#define    DD_DEV_TYPE_USB_CHECK()\
	if (dd_dev_type_usb_get() != lib_dd_dev_type_usb_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_USB mismatch(%x vs %x)\n", dd_dev_type_usb_get(), lib_dd_dev_type_usb_get());\
	}

uint32_t config_tpc_pa_map_get(void);
uint32_t lib_config_tpc_pa_map_get(void);
#define    CONFIG_TPC_PA_MAP_CHECK()\
	if (config_tpc_pa_map_get() != lib_config_tpc_pa_map_get()) {\
		BK_LOGW(TAG, "CONFIG_TPC_PA_MAP mismatch(%x vs %x)\n", config_tpc_pa_map_get(), lib_config_tpc_pa_map_get());\
	}

uint32_t config_iperf_test_get(void);
uint32_t lib_config_iperf_test_get(void);
#define    CONFIG_IPERF_TEST_CHECK()\
	if (config_iperf_test_get() != lib_config_iperf_test_get()) {\
		BK_LOGW(TAG, "CONFIG_IPERF_TEST mismatch(%x vs %x)\n", config_iperf_test_get(), lib_config_iperf_test_get());\
	}

uint32_t config_fully_hosted_get(void);
uint32_t lib_config_fully_hosted_get(void);
#define    CONFIG_FULLY_HOSTED_CHECK()\
	if (config_fully_hosted_get() != lib_config_fully_hosted_get()) {\
		BK_LOGW(TAG, "CONFIG_FULLY_HOSTED mismatch(%x vs %x)\n", config_fully_hosted_get(), lib_config_fully_hosted_get());\
	}

uint32_t config_soc_bk7231u_get(void);
uint32_t lib_config_soc_bk7231u_get(void);
#define    CONFIG_SOC_BK7231U_CHECK()\
	if (config_soc_bk7231u_get() != lib_config_soc_bk7231u_get()) {\
		BK_LOGW(TAG, "CONFIG_SOC_BK7231U mismatch(%x vs %x)\n", config_soc_bk7231u_get(), lib_config_soc_bk7231u_get());\
	}

uint32_t config_sta_ps_get(void);
uint32_t lib_config_sta_ps_get(void);
#define    CONFIG_STA_PS_CHECK()\
	if (config_sta_ps_get() != lib_config_sta_ps_get()) {\
		BK_LOGW(TAG, "CONFIG_STA_PS mismatch(%x vs %x)\n", config_sta_ps_get(), lib_config_sta_ps_get());\
	}

uint32_t config_spi_dma_get(void);
uint32_t lib_config_spi_dma_get(void);
#define    CONFIG_SPI_DMA_CHECK()\
	if (config_spi_dma_get() != lib_config_spi_dma_get()) {\
		BK_LOGW(TAG, "CONFIG_SPI_DMA mismatch(%x vs %x)\n", config_spi_dma_get(), lib_config_spi_dma_get());\
	}

uint32_t dd_dev_type_rf_get(void);
uint32_t lib_dd_dev_type_rf_get(void);
#define    DD_DEV_TYPE_RF_CHECK()\
	if (dd_dev_type_rf_get() != lib_dd_dev_type_rf_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_RF mismatch(%x vs %x)\n", dd_dev_type_rf_get(), lib_dd_dev_type_rf_get());\
	}

uint32_t config_ap_monitor_coexist_get(void);
uint32_t lib_config_ap_monitor_coexist_get(void);
#define    CONFIG_AP_MONITOR_COEXIST_CHECK()\
	if (config_ap_monitor_coexist_get() != lib_config_ap_monitor_coexist_get()) {\
		BK_LOGW(TAG, "CONFIG_AP_MONITOR_COEXIST mismatch(%x vs %x)\n", config_ap_monitor_coexist_get(), lib_config_ap_monitor_coexist_get());\
	}

uint32_t config_ap_ht_ie_get(void);
uint32_t lib_config_ap_ht_ie_get(void);
#define    CONFIG_AP_HT_IE_CHECK()\
	if (config_ap_ht_ie_get() != lib_config_ap_ht_ie_get()) {\
		BK_LOGW(TAG, "CONFIG_AP_HT_IE mismatch(%x vs %x)\n", config_ap_ht_ie_get(), lib_config_ap_ht_ie_get());\
	}

uint32_t config_pta_get(void);
uint32_t lib_config_pta_get(void);
#define    CONFIG_PTA_CHECK()\
	if (config_pta_get() != lib_config_pta_get()) {\
		BK_LOGW(TAG, "CONFIG_PTA mismatch(%x vs %x)\n", config_pta_get(), lib_config_pta_get());\
	}

uint32_t config_base_mac_from_nvs_get(void);
uint32_t lib_config_base_mac_from_nvs_get(void);
#define    CONFIG_BASE_MAC_FROM_NVS_CHECK()\
	if (config_base_mac_from_nvs_get() != lib_config_base_mac_from_nvs_get()) {\
		BK_LOGW(TAG, "CONFIG_BASE_MAC_FROM_NVS mismatch(%x vs %x)\n", config_base_mac_from_nvs_get(), lib_config_base_mac_from_nvs_get());\
	}

uint32_t config_wfa_ca_get(void);
uint32_t lib_config_wfa_ca_get(void);
#define    CONFIG_WFA_CA_CHECK()\
	if (config_wfa_ca_get() != lib_config_wfa_ca_get()) {\
		BK_LOGW(TAG, "CONFIG_WFA_CA mismatch(%x vs %x)\n", config_wfa_ca_get(), lib_config_wfa_ca_get());\
	}

uint32_t config_wifi_p2p_get(void);
uint32_t lib_config_wifi_p2p_get(void);
#define    CONFIG_WIFI_P2P_CHECK()\
	if (config_wifi_p2p_get() != lib_config_wifi_p2p_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_P2P mismatch(%x vs %x)\n", config_wifi_p2p_get(), lib_config_wifi_p2p_get());\
	}

uint32_t config_deep_ps_get(void);
uint32_t lib_config_deep_ps_get(void);
#define    CONFIG_DEEP_PS_CHECK()\
	if (config_deep_ps_get() != lib_config_deep_ps_get()) {\
		BK_LOGW(TAG, "CONFIG_DEEP_PS mismatch(%x vs %x)\n", config_deep_ps_get(), lib_config_deep_ps_get());\
	}

uint32_t dd_dev_type_i2s_get(void);
uint32_t lib_dd_dev_type_i2s_get(void);
#define    DD_DEV_TYPE_I2S_CHECK()\
	if (dd_dev_type_i2s_get() != lib_dd_dev_type_i2s_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_I2S mismatch(%x vs %x)\n", dd_dev_type_i2s_get(), lib_dd_dev_type_i2s_get());\
	}

uint32_t config_xtal_freq_40m_get(void);
uint32_t lib_config_xtal_freq_40m_get(void);
#define    CONFIG_XTAL_FREQ_40M_CHECK()\
	if (config_xtal_freq_40m_get() != lib_config_xtal_freq_40m_get()) {\
		BK_LOGW(TAG, "CONFIG_XTAL_FREQ_40M mismatch(%x vs %x)\n", config_xtal_freq_40m_get(), lib_config_xtal_freq_40m_get());\
	}

uint32_t config_debug_firmware_get(void);
uint32_t lib_config_debug_firmware_get(void);
#define    CONFIG_DEBUG_FIRMWARE_CHECK()\
	if (config_debug_firmware_get() != lib_config_debug_firmware_get()) {\
		BK_LOGW(TAG, "CONFIG_DEBUG_FIRMWARE mismatch(%x vs %x)\n", config_debug_firmware_get(), lib_config_debug_firmware_get());\
	}

uint32_t config_lwip_2_1_2_get(void);
uint32_t lib_config_lwip_2_1_2_get(void);
#define    CONFIG_LWIP_2_1_2_CHECK()\
	if (config_lwip_2_1_2_get() != lib_config_lwip_2_1_2_get()) {\
		BK_LOGW(TAG, "CONFIG_LWIP_2_1_2 mismatch(%x vs %x)\n", config_lwip_2_1_2_get(), lib_config_lwip_2_1_2_get());\
	}

uint32_t config_usb_hid_get(void);
uint32_t lib_config_usb_hid_get(void);
#define    CONFIG_USB_HID_CHECK()\
	if (config_usb_hid_get() != lib_config_usb_hid_get()) {\
		BK_LOGW(TAG, "CONFIG_USB_HID mismatch(%x vs %x)\n", config_usb_hid_get(), lib_config_usb_hid_get());\
	}

uint32_t dd_dev_type_end_get(void);
uint32_t lib_dd_dev_type_end_get(void);
#define    DD_DEV_TYPE_END_CHECK()\
	if (dd_dev_type_end_get() != lib_dd_dev_type_end_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_END mismatch(%x vs %x)\n", dd_dev_type_end_get(), lib_dd_dev_type_end_get());\
	}

uint32_t config_usb_msd_get(void);
uint32_t lib_config_usb_msd_get(void);
#define    CONFIG_USB_MSD_CHECK()\
	if (config_usb_msd_get() != lib_config_usb_msd_get()) {\
		BK_LOGW(TAG, "CONFIG_USB_MSD mismatch(%x vs %x)\n", config_usb_msd_get(), lib_config_usb_msd_get());\
	}

uint32_t dd_dev_type_spidma_get(void);
uint32_t lib_dd_dev_type_spidma_get(void);
#define    DD_DEV_TYPE_SPIDMA_CHECK()\
	if (dd_dev_type_spidma_get() != lib_dd_dev_type_spidma_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SPIDMA mismatch(%x vs %x)\n", dd_dev_type_spidma_get(), lib_dd_dev_type_spidma_get());\
	}

uint32_t dd_dev_type_flash_get(void);
uint32_t lib_dd_dev_type_flash_get(void);
#define    DD_DEV_TYPE_FLASH_CHECK()\
	if (dd_dev_type_flash_get() != lib_dd_dev_type_flash_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_FLASH mismatch(%x vs %x)\n", dd_dev_type_flash_get(), lib_dd_dev_type_flash_get());\
	}

uint32_t config_wpa3_get(void);
uint32_t lib_config_wpa3_get(void);
#define    CONFIG_WPA3_CHECK()\
	if (config_wpa3_get() != lib_config_wpa3_get()) {\
		BK_LOGW(TAG, "CONFIG_WPA3 mismatch(%x vs %x)\n", config_wpa3_get(), lib_config_wpa3_get());\
	}

uint32_t config_xtal_freq_26m_get(void);
uint32_t lib_config_xtal_freq_26m_get(void);
#define    CONFIG_XTAL_FREQ_26M_CHECK()\
	if (config_xtal_freq_26m_get() != lib_config_xtal_freq_26m_get()) {\
		BK_LOGW(TAG, "CONFIG_XTAL_FREQ_26M mismatch(%x vs %x)\n", config_xtal_freq_26m_get(), lib_config_xtal_freq_26m_get());\
	}

uint32_t config_ble_scan_num_get(void);
uint32_t lib_config_ble_scan_num_get(void);
#define    CONFIG_BLE_SCAN_NUM_CHECK()\
	if (config_ble_scan_num_get() != lib_config_ble_scan_num_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_SCAN_NUM mismatch(%x vs %x)\n", config_ble_scan_num_get(), lib_config_ble_scan_num_get());\
	}

uint32_t config_xtal_freq_get(void);
uint32_t lib_config_xtal_freq_get(void);
#define    CONFIG_XTAL_FREQ_CHECK()\
	if (config_xtal_freq_get() != lib_config_xtal_freq_get()) {\
		BK_LOGW(TAG, "CONFIG_XTAL_FREQ mismatch(%x vs %x)\n", config_xtal_freq_get(), lib_config_xtal_freq_get());\
	}

uint32_t config_dcache_get(void);
uint32_t lib_config_dcache_get(void);
#define    CONFIG_DCACHE_CHECK()\
	if (config_dcache_get() != lib_config_dcache_get()) {\
		BK_LOGW(TAG, "CONFIG_DCACHE mismatch(%x vs %x)\n", config_dcache_get(), lib_config_dcache_get());\
	}

uint32_t dd_dev_type_ble_get(void);
uint32_t lib_dd_dev_type_ble_get(void);
#define    DD_DEV_TYPE_BLE_CHECK()\
	if (dd_dev_type_ble_get() != lib_dd_dev_type_ble_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_BLE mismatch(%x vs %x)\n", dd_dev_type_ble_get(), lib_dd_dev_type_ble_get());\
	}

uint32_t config_dsp_src_add_get(void);
uint32_t lib_config_dsp_src_add_get(void);
#define    CONFIG_DSP_SRC_ADD_CHECK()\
	if (config_dsp_src_add_get() != lib_config_dsp_src_add_get()) {\
		BK_LOGW(TAG, "CONFIG_DSP_SRC_ADD mismatch(%x vs %x)\n", config_dsp_src_add_get(), lib_config_dsp_src_add_get());\
	}

uint32_t dd_dev_type_gdma_get(void);
uint32_t lib_dd_dev_type_gdma_get(void);
#define    DD_DEV_TYPE_GDMA_CHECK()\
	if (dd_dev_type_gdma_get() != lib_dd_dev_type_gdma_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_GDMA mismatch(%x vs %x)\n", dd_dev_type_gdma_get(), lib_dd_dev_type_gdma_get());\
	}

uint32_t config_ota_tftp_get(void);
uint32_t lib_config_ota_tftp_get(void);
#define    CONFIG_OTA_TFTP_CHECK()\
	if (config_ota_tftp_get() != lib_config_ota_tftp_get()) {\
		BK_LOGW(TAG, "CONFIG_OTA_TFTP mismatch(%x vs %x)\n", config_ota_tftp_get(), lib_config_ota_tftp_get());\
	}

uint32_t config_msdu_resv_tail_len_get(void);
uint32_t lib_config_msdu_resv_tail_len_get(void);
#define    CONFIG_MSDU_RESV_TAIL_LEN_CHECK()\
	if (config_msdu_resv_tail_len_get() != lib_config_msdu_resv_tail_len_get()) {\
		BK_LOGW(TAG, "CONFIG_MSDU_RESV_TAIL_LEN mismatch(%x vs %x)\n", config_msdu_resv_tail_len_get(), lib_config_msdu_resv_tail_len_get());\
	}

uint32_t config_bk_assert_reboot_get(void);
uint32_t lib_config_bk_assert_reboot_get(void);
#define    CONFIG_BK_ASSERT_REBOOT_CHECK()\
	if (config_bk_assert_reboot_get() != lib_config_bk_assert_reboot_get()) {\
		BK_LOGW(TAG, "CONFIG_BK_ASSERT_REBOOT mismatch(%x vs %x)\n", config_bk_assert_reboot_get(), lib_config_bk_assert_reboot_get());\
	}

uint32_t config_spi_mst_psram_get(void);
uint32_t lib_config_spi_mst_psram_get(void);
#define    CONFIG_SPI_MST_PSRAM_CHECK()\
	if (config_spi_mst_psram_get() != lib_config_spi_mst_psram_get()) {\
		BK_LOGW(TAG, "CONFIG_SPI_MST_PSRAM mismatch(%x vs %x)\n", config_spi_mst_psram_get(), lib_config_spi_mst_psram_get());\
	}

uint32_t dd_dev_type_pwm_get(void);
uint32_t lib_dd_dev_type_pwm_get(void);
#define    DD_DEV_TYPE_PWM_CHECK()\
	if (dd_dev_type_pwm_get() != lib_dd_dev_type_pwm_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_PWM mismatch(%x vs %x)\n", dd_dev_type_pwm_get(), lib_dd_dev_type_pwm_get());\
	}

uint32_t config_bkreg_get(void);
uint32_t lib_config_bkreg_get(void);
#define    CONFIG_BKREG_CHECK()\
	if (config_bkreg_get() != lib_config_bkreg_get()) {\
		BK_LOGW(TAG, "CONFIG_BKREG mismatch(%x vs %x)\n", config_bkreg_get(), lib_config_bkreg_get());\
	}

uint32_t dd_dev_type_bt_get(void);
uint32_t lib_dd_dev_type_bt_get(void);
#define    DD_DEV_TYPE_BT_CHECK()\
	if (dd_dev_type_bt_get() != lib_dd_dev_type_bt_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_BT mismatch(%x vs %x)\n", dd_dev_type_bt_get(), lib_dd_dev_type_bt_get());\
	}

uint32_t dd_dev_type_trng_get(void);
uint32_t lib_dd_dev_type_trng_get(void);
#define    DD_DEV_TYPE_TRNG_CHECK()\
	if (dd_dev_type_trng_get() != lib_dd_dev_type_trng_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_TRNG mismatch(%x vs %x)\n", dd_dev_type_trng_get(), lib_dd_dev_type_trng_get());\
	}

uint32_t config_demo_test_get(void);
uint32_t lib_config_demo_test_get(void);
#define    CONFIG_DEMO_TEST_CHECK()\
	if (config_demo_test_get() != lib_config_demo_test_get()) {\
		BK_LOGW(TAG, "CONFIG_DEMO_TEST mismatch(%x vs %x)\n", config_demo_test_get(), lib_config_demo_test_get());\
	}

uint32_t config_task_lwip_prio_get(void);
uint32_t lib_config_task_lwip_prio_get(void);
#define    CONFIG_TASK_LWIP_PRIO_CHECK()\
	if (config_task_lwip_prio_get() != lib_config_task_lwip_prio_get()) {\
		BK_LOGW(TAG, "CONFIG_TASK_LWIP_PRIO mismatch(%x vs %x)\n", config_task_lwip_prio_get(), lib_config_task_lwip_prio_get());\
	}

uint32_t config_i2c2_test_get(void);
uint32_t lib_config_i2c2_test_get(void);
#define    CONFIG_I2C2_TEST_CHECK()\
	if (config_i2c2_test_get() != lib_config_i2c2_test_get()) {\
		BK_LOGW(TAG, "CONFIG_I2C2_TEST mismatch(%x vs %x)\n", config_i2c2_test_get(), lib_config_i2c2_test_get());\
	}

uint32_t config_rf_policy_co_req_get(void);
uint32_t lib_config_rf_policy_co_req_get(void);
#define    CONFIG_RF_POLICY_CO_REQ_CHECK()\
	if (config_rf_policy_co_req_get() != lib_config_rf_policy_co_req_get()) {\
		BK_LOGW(TAG, "CONFIG_RF_POLICY_CO_REQ mismatch(%x vs %x)\n", config_rf_policy_co_req_get(), lib_config_rf_policy_co_req_get());\
	}

uint32_t config_airkiss_test_get(void);
uint32_t lib_config_airkiss_test_get(void);
#define    CONFIG_AIRKISS_TEST_CHECK()\
	if (config_airkiss_test_get() != lib_config_airkiss_test_get()) {\
		BK_LOGW(TAG, "CONFIG_AIRKISS_TEST mismatch(%x vs %x)\n", config_airkiss_test_get(), lib_config_airkiss_test_get());\
	}

uint32_t dd_dev_type_spi_get(void);
uint32_t lib_dd_dev_type_spi_get(void);
#define    DD_DEV_TYPE_SPI_CHECK()\
	if (dd_dev_type_spi_get() != lib_dd_dev_type_spi_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SPI mismatch(%x vs %x)\n", dd_dev_type_spi_get(), lib_dd_dev_type_spi_get());\
	}

uint32_t config_ap_vht_get(void);
uint32_t lib_config_ap_vht_get(void);
#define    CONFIG_AP_VHT_CHECK()\
	if (config_ap_vht_get() != lib_config_ap_vht_get()) {\
		BK_LOGW(TAG, "CONFIG_AP_VHT mismatch(%x vs %x)\n", config_ap_vht_get(), lib_config_ap_vht_get());\
	}

uint32_t dd_dev_type_spi2_get(void);
uint32_t lib_dd_dev_type_spi2_get(void);
#define    DD_DEV_TYPE_SPI2_CHECK()\
	if (dd_dev_type_spi2_get() != lib_dd_dev_type_spi2_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SPI2 mismatch(%x vs %x)\n", dd_dev_type_spi2_get(), lib_dd_dev_type_spi2_get());\
	}

uint32_t config_ftpd_upgrade_get(void);
uint32_t lib_config_ftpd_upgrade_get(void);
#define    CONFIG_FTPD_UPGRADE_CHECK()\
	if (config_ftpd_upgrade_get() != lib_config_ftpd_upgrade_get()) {\
		BK_LOGW(TAG, "CONFIG_FTPD_UPGRADE mismatch(%x vs %x)\n", config_ftpd_upgrade_get(), lib_config_ftpd_upgrade_get());\
	}

uint32_t dd_dev_type_cal_get(void);
uint32_t lib_dd_dev_type_cal_get(void);
#define    DD_DEV_TYPE_CAL_CHECK()\
	if (dd_dev_type_cal_get() != lib_dd_dev_type_cal_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_CAL mismatch(%x vs %x)\n", dd_dev_type_cal_get(), lib_dd_dev_type_cal_get());\
	}

uint32_t config_bt_mesh_health_cli_get(void);
uint32_t lib_config_bt_mesh_health_cli_get(void);
#define    CONFIG_BT_MESH_HEALTH_CLI_CHECK()\
	if (config_bt_mesh_health_cli_get() != lib_config_bt_mesh_health_cli_get()) {\
		BK_LOGW(TAG, "CONFIG_BT_MESH_HEALTH_CLI mismatch(%x vs %x)\n", config_bt_mesh_health_cli_get(), lib_config_bt_mesh_health_cli_get());\
	}

uint32_t config_dsp_get(void);
uint32_t lib_config_dsp_get(void);
#define    CONFIG_DSP_CHECK()\
	if (config_dsp_get() != lib_config_dsp_get()) {\
		BK_LOGW(TAG, "CONFIG_DSP mismatch(%x vs %x)\n", config_dsp_get(), lib_config_dsp_get());\
	}

uint32_t config_sdio_get(void);
uint32_t lib_config_sdio_get(void);
#define    CONFIG_SDIO_CHECK()\
	if (config_sdio_get() != lib_config_sdio_get()) {\
		BK_LOGW(TAG, "CONFIG_SDIO mismatch(%x vs %x)\n", config_sdio_get(), lib_config_sdio_get());\
	}

uint32_t config_mcu_ps_get(void);
uint32_t lib_config_mcu_ps_get(void);
#define    CONFIG_MCU_PS_CHECK()\
	if (config_mcu_ps_get() != lib_config_mcu_ps_get()) {\
		BK_LOGW(TAG, "CONFIG_MCU_PS mismatch(%x vs %x)\n", config_mcu_ps_get(), lib_config_mcu_ps_get());\
	}

uint32_t config_wifi_fast_connect_get(void);
uint32_t lib_config_wifi_fast_connect_get(void);
#define    CONFIG_WIFI_FAST_CONNECT_CHECK()\
	if (config_wifi_fast_connect_get() != lib_config_wifi_fast_connect_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_FAST_CONNECT mismatch(%x vs %x)\n", config_wifi_fast_connect_get(), lib_config_wifi_fast_connect_get());\
	}

uint32_t dd_dev_type_start_get(void);
uint32_t lib_dd_dev_type_start_get(void);
#define    DD_DEV_TYPE_START_CHECK()\
	if (dd_dev_type_start_get() != lib_dd_dev_type_start_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_START mismatch(%x vs %x)\n", dd_dev_type_start_get(), lib_dd_dev_type_start_get());\
	}

uint32_t config_tcp_server_test_get(void);
uint32_t lib_config_tcp_server_test_get(void);
#define    CONFIG_TCP_SERVER_TEST_CHECK()\
	if (config_tcp_server_test_get() != lib_config_tcp_server_test_get()) {\
		BK_LOGW(TAG, "CONFIG_TCP_SERVER_TEST mismatch(%x vs %x)\n", config_tcp_server_test_get(), lib_config_tcp_server_test_get());\
	}

uint32_t config_sys_reduce_normal_power_get(void);
uint32_t lib_config_sys_reduce_normal_power_get(void);
#define    CONFIG_SYS_REDUCE_NORMAL_POWER_CHECK()\
	if (config_sys_reduce_normal_power_get() != lib_config_sys_reduce_normal_power_get()) {\
		BK_LOGW(TAG, "CONFIG_SYS_REDUCE_NORMAL_POWER mismatch(%x vs %x)\n", config_sys_reduce_normal_power_get(), lib_config_sys_reduce_normal_power_get());\
	}

uint32_t config_qspi_get(void);
uint32_t lib_config_qspi_get(void);
#define    CONFIG_QSPI_CHECK()\
	if (config_qspi_get() != lib_config_qspi_get()) {\
		BK_LOGW(TAG, "CONFIG_QSPI mismatch(%x vs %x)\n", config_qspi_get(), lib_config_qspi_get());\
	}

uint32_t dd_dev_type_sdcard_get(void);
uint32_t lib_dd_dev_type_sdcard_get(void);
#define    DD_DEV_TYPE_SDCARD_CHECK()\
	if (dd_dev_type_sdcard_get() != lib_dd_dev_type_sdcard_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SDCARD mismatch(%x vs %x)\n", dd_dev_type_sdcard_get(), lib_dd_dev_type_sdcard_get());\
	}

uint32_t dd_dev_type_none_get(void);
uint32_t lib_dd_dev_type_none_get(void);
#define    DD_DEV_TYPE_NONE_CHECK()\
	if (dd_dev_type_none_get() != lib_dd_dev_type_none_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_NONE mismatch(%x vs %x)\n", dd_dev_type_none_get(), lib_dd_dev_type_none_get());\
	}

uint32_t config_sdio_trans_get(void);
uint32_t lib_config_sdio_trans_get(void);
#define    CONFIG_SDIO_TRANS_CHECK()\
	if (config_sdio_trans_get() != lib_config_sdio_trans_get()) {\
		BK_LOGW(TAG, "CONFIG_SDIO_TRANS mismatch(%x vs %x)\n", config_sdio_trans_get(), lib_config_sdio_trans_get());\
	}

uint32_t dd_dev_type_gpio_get(void);
uint32_t lib_dd_dev_type_gpio_get(void);
#define    DD_DEV_TYPE_GPIO_CHECK()\
	if (dd_dev_type_gpio_get() != lib_dd_dev_type_gpio_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_GPIO mismatch(%x vs %x)\n", dd_dev_type_gpio_get(), lib_dd_dev_type_gpio_get());\
	}

uint32_t config_rx_sense_test_get(void);
uint32_t lib_config_rx_sense_test_get(void);
#define    CONFIG_RX_SENSE_TEST_CHECK()\
	if (config_rx_sense_test_get() != lib_config_rx_sense_test_get()) {\
		BK_LOGW(TAG, "CONFIG_RX_SENSE_TEST mismatch(%x vs %x)\n", config_rx_sense_test_get(), lib_config_rx_sense_test_get());\
	}

uint32_t config_ble_adv_num_get(void);
uint32_t lib_config_ble_adv_num_get(void);
#define    CONFIG_BLE_ADV_NUM_CHECK()\
	if (config_ble_adv_num_get() != lib_config_ble_adv_num_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_ADV_NUM mismatch(%x vs %x)\n", config_ble_adv_num_get(), lib_config_ble_adv_num_get());\
	}

uint32_t config_lwip_mem_default_get(void);
uint32_t lib_config_lwip_mem_default_get(void);
#define    CONFIG_LWIP_MEM_DEFAULT_CHECK()\
	if (config_lwip_mem_default_get() != lib_config_lwip_mem_default_get()) {\
		BK_LOGW(TAG, "CONFIG_LWIP_MEM_DEFAULT mismatch(%x vs %x)\n", config_lwip_mem_default_get(), lib_config_lwip_mem_default_get());\
	}

uint32_t config_power_table_get(void);
uint32_t lib_config_power_table_get(void);
#define    CONFIG_POWER_TABLE_CHECK()\
	if (config_power_table_get() != lib_config_power_table_get()) {\
		BK_LOGW(TAG, "CONFIG_POWER_TABLE mismatch(%x vs %x)\n", config_power_table_get(), lib_config_power_table_get());\
	}

uint32_t dd_dev_type_wdt_get(void);
uint32_t lib_dd_dev_type_wdt_get(void);
#define    DD_DEV_TYPE_WDT_CHECK()\
	if (dd_dev_type_wdt_get() != lib_dd_dev_type_wdt_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_WDT mismatch(%x vs %x)\n", dd_dev_type_wdt_get(), lib_dd_dev_type_wdt_get());\
	}

uint32_t dd_dev_type_i2c1_get(void);
uint32_t lib_dd_dev_type_i2c1_get(void);
#define    DD_DEV_TYPE_I2C1_CHECK()\
	if (dd_dev_type_i2c1_get() != lib_dd_dev_type_i2c1_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_I2C1 mismatch(%x vs %x)\n", dd_dev_type_i2c1_get(), lib_dd_dev_type_i2c1_get());\
	}

uint32_t config_bk_assert_halt_get(void);
uint32_t lib_config_bk_assert_halt_get(void);
#define    CONFIG_BK_ASSERT_HALT_CHECK()\
	if (config_bk_assert_halt_get() != lib_config_bk_assert_halt_get()) {\
		BK_LOGW(TAG, "CONFIG_BK_ASSERT_HALT mismatch(%x vs %x)\n", config_bk_assert_halt_get(), lib_config_bk_assert_halt_get());\
	}

uint32_t config_saradc_cali_get(void);
uint32_t lib_config_saradc_cali_get(void);
#define    CONFIG_SARADC_CALI_CHECK()\
	if (config_saradc_cali_get() != lib_config_saradc_cali_get()) {\
		BK_LOGW(TAG, "CONFIG_SARADC_CALI mismatch(%x vs %x)\n", config_saradc_cali_get(), lib_config_saradc_cali_get());\
	}

uint32_t config_task_reconnect_prio_get(void);
uint32_t lib_config_task_reconnect_prio_get(void);
#define    CONFIG_TASK_RECONNECT_PRIO_CHECK()\
	if (config_task_reconnect_prio_get() != lib_config_task_reconnect_prio_get()) {\
		BK_LOGW(TAG, "CONFIG_TASK_RECONNECT_PRIO mismatch(%x vs %x)\n", config_task_reconnect_prio_get(), lib_config_task_reconnect_prio_get());\
	}

uint32_t config_wifi_scan_ch_time_get(void);
uint32_t lib_config_wifi_scan_ch_time_get(void);
#define    CONFIG_WIFI_SCAN_CH_TIME_CHECK()\
	if (config_wifi_scan_ch_time_get() != lib_config_wifi_scan_ch_time_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_SCAN_CH_TIME mismatch(%x vs %x)\n", config_wifi_scan_ch_time_get(), lib_config_wifi_scan_ch_time_get());\
	}

uint32_t config_i2c1_test_get(void);
uint32_t lib_config_i2c1_test_get(void);
#define    CONFIG_I2C1_TEST_CHECK()\
	if (config_i2c1_test_get() != lib_config_i2c1_test_get()) {\
		BK_LOGW(TAG, "CONFIG_I2C1_TEST mismatch(%x vs %x)\n", config_i2c1_test_get(), lib_config_i2c1_test_get());\
	}

uint32_t config_freertos_v10_get(void);
uint32_t lib_config_freertos_v10_get(void);
#define    CONFIG_FREERTOS_V10_CHECK()\
	if (config_freertos_v10_get() != lib_config_freertos_v10_get()) {\
		BK_LOGW(TAG, "CONFIG_FREERTOS_V10 mismatch(%x vs %x)\n", config_freertos_v10_get(), lib_config_freertos_v10_get());\
	}

uint32_t config_jtag_get(void);
uint32_t lib_config_jtag_get(void);
#define    CONFIG_JTAG_CHECK()\
	if (config_jtag_get() != lib_config_jtag_get()) {\
		BK_LOGW(TAG, "CONFIG_JTAG mismatch(%x vs %x)\n", config_jtag_get(), lib_config_jtag_get());\
	}

uint32_t dd_dev_type_spi3_get(void);
uint32_t lib_dd_dev_type_spi3_get(void);
#define    DD_DEV_TYPE_SPI3_CHECK()\
	if (dd_dev_type_spi3_get() != lib_dd_dev_type_spi3_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_SPI3 mismatch(%x vs %x)\n", dd_dev_type_spi3_get(), lib_dd_dev_type_spi3_get());\
	}

uint32_t config_base_mac_from_efuse_get(void);
uint32_t lib_config_base_mac_from_efuse_get(void);
#define    CONFIG_BASE_MAC_FROM_EFUSE_CHECK()\
	if (config_base_mac_from_efuse_get() != lib_config_base_mac_from_efuse_get()) {\
		BK_LOGW(TAG, "CONFIG_BASE_MAC_FROM_EFUSE mismatch(%x vs %x)\n", config_base_mac_from_efuse_get(), lib_config_base_mac_from_efuse_get());\
	}

uint32_t config_real_sdio_get(void);
uint32_t lib_config_real_sdio_get(void);
#define    CONFIG_REAL_SDIO_CHECK()\
	if (config_real_sdio_get() != lib_config_real_sdio_get()) {\
		BK_LOGW(TAG, "CONFIG_REAL_SDIO mismatch(%x vs %x)\n", config_real_sdio_get(), lib_config_real_sdio_get());\
	}

uint32_t config_sdio_block_512_get(void);
uint32_t lib_config_sdio_block_512_get(void);
#define    CONFIG_SDIO_BLOCK_512_CHECK()\
	if (config_sdio_block_512_get() != lib_config_sdio_block_512_get()) {\
		BK_LOGW(TAG, "CONFIG_SDIO_BLOCK_512 mismatch(%x vs %x)\n", config_sdio_block_512_get(), lib_config_sdio_block_512_get());\
	}

uint32_t config_ble_host_zephyr_get(void);
uint32_t lib_config_ble_host_zephyr_get(void);
#define    CONFIG_BLE_HOST_ZEPHYR_CHECK()\
	if (config_ble_host_zephyr_get() != lib_config_ble_host_zephyr_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_HOST_ZEPHYR mismatch(%x vs %x)\n", config_ble_host_zephyr_get(), lib_config_ble_host_zephyr_get());\
	}

uint32_t config_soc_bk7251_get(void);
uint32_t lib_config_soc_bk7251_get(void);
#define    CONFIG_SOC_BK7251_CHECK()\
	if (config_soc_bk7251_get() != lib_config_soc_bk7251_get()) {\
		BK_LOGW(TAG, "CONFIG_SOC_BK7251 mismatch(%x vs %x)\n", config_soc_bk7251_get(), lib_config_soc_bk7251_get());\
	}

uint32_t config_wifi4_get(void);
uint32_t lib_config_wifi4_get(void);
#define    CONFIG_WIFI4_CHECK()\
	if (config_wifi4_get() != lib_config_wifi4_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI4 mismatch(%x vs %x)\n", config_wifi4_get(), lib_config_wifi4_get());\
	}

uint32_t config_soc_bk7231_get(void);
uint32_t lib_config_soc_bk7231_get(void);
#define    CONFIG_SOC_BK7231_CHECK()\
	if (config_soc_bk7231_get() != lib_config_soc_bk7231_get()) {\
		BK_LOGW(TAG, "CONFIG_SOC_BK7231 mismatch(%x vs %x)\n", config_soc_bk7231_get(), lib_config_soc_bk7231_get());\
	}

uint32_t config_usb_charge_get(void);
uint32_t lib_config_usb_charge_get(void);
#define    CONFIG_USB_CHARGE_CHECK()\
	if (config_usb_charge_get() != lib_config_usb_charge_get()) {\
		BK_LOGW(TAG, "CONFIG_USB_CHARGE mismatch(%x vs %x)\n", config_usb_charge_get(), lib_config_usb_charge_get());\
	}

uint32_t config_ap_ps_get(void);
uint32_t lib_config_ap_ps_get(void);
#define    CONFIG_AP_PS_CHECK()\
	if (config_ap_ps_get() != lib_config_ap_ps_get()) {\
		BK_LOGW(TAG, "CONFIG_AP_PS mismatch(%x vs %x)\n", config_ap_ps_get(), lib_config_ap_ps_get());\
	}

uint32_t config_less_memery_in_rwnx_get(void);
uint32_t lib_config_less_memery_in_rwnx_get(void);
#define    CONFIG_LESS_MEMERY_IN_RWNX_CHECK()\
	if (config_less_memery_in_rwnx_get() != lib_config_less_memery_in_rwnx_get()) {\
		BK_LOGW(TAG, "CONFIG_LESS_MEMERY_IN_RWNX mismatch(%x vs %x)\n", config_less_memery_in_rwnx_get(), lib_config_less_memery_in_rwnx_get());\
	}

uint32_t config_uart_dma_get(void);
uint32_t lib_config_uart_dma_get(void);
#define    CONFIG_UART_DMA_CHECK()\
	if (config_uart_dma_get() != lib_config_uart_dma_get()) {\
		BK_LOGW(TAG, "CONFIG_UART_DMA mismatch(%x vs %x)\n", config_uart_dma_get(), lib_config_uart_dma_get());\
	}

uint32_t config_bk7221_mdm_watchdog_patch_get(void);
uint32_t lib_config_bk7221_mdm_watchdog_patch_get(void);
#define    CONFIG_BK7221_MDM_WATCHDOG_PATCH_CHECK()\
	if (config_bk7221_mdm_watchdog_patch_get() != lib_config_bk7221_mdm_watchdog_patch_get()) {\
		BK_LOGW(TAG, "CONFIG_BK7221_MDM_WATCHDOG_PATCH mismatch(%x vs %x)\n", config_bk7221_mdm_watchdog_patch_get(), lib_config_bk7221_mdm_watchdog_patch_get());\
	}

uint32_t config_spidma_get(void);
uint32_t lib_config_spidma_get(void);
#define    CONFIG_SPIDMA_CHECK()\
	if (config_spidma_get() != lib_config_spidma_get()) {\
		BK_LOGW(TAG, "CONFIG_SPIDMA mismatch(%x vs %x)\n", config_spidma_get(), lib_config_spidma_get());\
	}

uint32_t config_ble_4_2_get(void);
uint32_t lib_config_ble_4_2_get(void);
#define    CONFIG_BLE_4_2_CHECK()\
	if (config_ble_4_2_get() != lib_config_ble_4_2_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_4_2 mismatch(%x vs %x)\n", config_ble_4_2_get(), lib_config_ble_4_2_get());\
	}

uint32_t config_ble_init_num_get(void);
uint32_t lib_config_ble_init_num_get(void);
#define    CONFIG_BLE_INIT_NUM_CHECK()\
	if (config_ble_init_num_get() != lib_config_ble_init_num_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE_INIT_NUM mismatch(%x vs %x)\n", config_ble_init_num_get(), lib_config_ble_init_num_get());\
	}

uint32_t config_support_irda_get(void);
uint32_t lib_config_support_irda_get(void);
#define    CONFIG_SUPPORT_IRDA_CHECK()\
	if (config_support_irda_get() != lib_config_support_irda_get()) {\
		BK_LOGW(TAG, "CONFIG_SUPPORT_IRDA mismatch(%x vs %x)\n", config_support_irda_get(), lib_config_support_irda_get());\
	}

uint32_t config_app_demo_video_transfer_get(void);
uint32_t lib_config_app_demo_video_transfer_get(void);
#define    CONFIG_APP_DEMO_VIDEO_TRANSFER_CHECK()\
	if (config_app_demo_video_transfer_get() != lib_config_app_demo_video_transfer_get()) {\
		BK_LOGW(TAG, "CONFIG_APP_DEMO_VIDEO_TRANSFER mismatch(%x vs %x)\n", config_app_demo_video_transfer_get(), lib_config_app_demo_video_transfer_get());\
	}

uint32_t config_tx_evm_test_get(void);
uint32_t lib_config_tx_evm_test_get(void);
#define    CONFIG_TX_EVM_TEST_CHECK()\
	if (config_tx_evm_test_get() != lib_config_tx_evm_test_get()) {\
		BK_LOGW(TAG, "CONFIG_TX_EVM_TEST mismatch(%x vs %x)\n", config_tx_evm_test_get(), lib_config_tx_evm_test_get());\
	}

uint32_t config_ap_probereq_cb_get(void);
uint32_t lib_config_ap_probereq_cb_get(void);
#define    CONFIG_AP_PROBEREQ_CB_CHECK()\
	if (config_ap_probereq_cb_get() != lib_config_ap_probereq_cb_get()) {\
		BK_LOGW(TAG, "CONFIG_AP_PROBEREQ_CB mismatch(%x vs %x)\n", config_ap_probereq_cb_get(), lib_config_ap_probereq_cb_get());\
	}

uint32_t config_pcm_resampler_get(void);
uint32_t lib_config_pcm_resampler_get(void);
#define    CONFIG_PCM_RESAMPLER_CHECK()\
	if (config_pcm_resampler_get() != lib_config_pcm_resampler_get()) {\
		BK_LOGW(TAG, "CONFIG_PCM_RESAMPLER mismatch(%x vs %x)\n", config_pcm_resampler_get(), lib_config_pcm_resampler_get());\
	}

uint32_t config_button_get(void);
uint32_t lib_config_button_get(void);
#define    CONFIG_BUTTON_CHECK()\
	if (config_button_get() != lib_config_button_get()) {\
		BK_LOGW(TAG, "CONFIG_BUTTON mismatch(%x vs %x)\n", config_button_get(), lib_config_button_get());\
	}

uint32_t dd_dev_type_i2c2_get(void);
uint32_t lib_dd_dev_type_i2c2_get(void);
#define    DD_DEV_TYPE_I2C2_CHECK()\
	if (dd_dev_type_i2c2_get() != lib_dd_dev_type_i2c2_get()) {\
		BK_LOGW(TAG, "DD_DEV_TYPE_I2C2 mismatch(%x vs %x)\n", dd_dev_type_i2c2_get(), lib_dd_dev_type_i2c2_get());\
	}

uint32_t config_ble_get(void);
uint32_t lib_config_ble_get(void);
#define    CONFIG_BLE_CHECK()\
	if (config_ble_get() != lib_config_ble_get()) {\
		BK_LOGW(TAG, "CONFIG_BLE mismatch(%x vs %x)\n", config_ble_get(), lib_config_ble_get());\
	}

uint32_t config_wifi_wps_get(void);
uint32_t lib_config_wifi_wps_get(void);
#define    CONFIG_WIFI_WPS_CHECK()\
	if (config_wifi_wps_get() != lib_config_wifi_wps_get()) {\
		BK_LOGW(TAG, "CONFIG_WIFI_WPS mismatch(%x vs %x)\n", config_wifi_wps_get(), lib_config_wifi_wps_get());\
	}
