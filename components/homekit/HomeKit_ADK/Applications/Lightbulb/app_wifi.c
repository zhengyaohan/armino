/* A beken Wi-Fi Accessory Implementation.
 * It includes Soft Access Point and Station Mode, ie
 * HAPPlatformSoftwareAccessPoint.c and HAPPlatformWiFiManager.c
 */
#include <string.h>
#include "include.h"
#include "str_pub.h"
#include "common.h"
#include "param_config.h"
#include "wlan_ui_pub.h"
#include "wlan_cli_pub.h"

#define lc_prf(fmt, ...) bk_printf("HK%s-%d: "fmt, __func__, __LINE__, ##__VA_ARGS__)

#define MAC_FMT "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ARG(a) \
	(uint8)(a[0]), (uint8)(a[1]), (uint8)(a[2]),\
		(uint8)(a[3]), (uint8)(a[4]), (uint8)(a[5])

#define WLAN_DEFAULT_IP         "192.168.0.1"
#define WLAN_DEFAULT_GW         "192.168.0.1"
#define WLAN_DEFAULT_MASK       "255.255.255.0"

static network_InitTypeDef_st wNetConfig;

/* "homekit sta/ap ssid key" */
static int app_homekit_conf_save(int argc, char **argv)
{
	int len;
	char *ssid, *key = NULL;

	ssid = argv[2];
	len = os_strlen(ssid);
	if (SSID_MAX_LEN < len) {
		lc_prf("ssid name more than 32 Bytes\r\n");
		return -1;
	}
	if (argv[3])
		key = argv[3];

	os_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

	os_strcpy((char *)wNetConfig.wifi_ssid, ssid);
	os_strcpy((char *)wNetConfig.wifi_key, key);
	wNetConfig.wifi_retry_interval = 100;

	if (!os_strcmp("ap", argv[1])) {
		wNetConfig.wifi_mode = BK_SOFT_AP;
		wNetConfig.dhcp_mode = DHCP_SERVER;
		os_strcpy((char *)wNetConfig.local_ip_addr, WLAN_DEFAULT_IP);
		os_strcpy((char *)wNetConfig.net_mask, WLAN_DEFAULT_MASK);
		os_strcpy((char *)wNetConfig.gateway_ip_addr, WLAN_DEFAULT_GW);
		os_strcpy((char *)wNetConfig.dns_server_ip_addr, WLAN_DEFAULT_GW);
	} else if (!os_strcmp("sta", argv[1])) {
		wNetConfig.wifi_mode = BK_STATION;
		wNetConfig.dhcp_mode = DHCP_CLIENT;
		lc_prf("wifi_mode = BK_STATION\r\n");
	}

	return 0;
}

static void hk_command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;

	if (3 > argc) {
		lc_prf("argv ERR(argc %d)\r\n", argc);
	}
	if ((os_strcmp("sta", argv[1])) && (os_strcmp("ap", argv[1]))) {
		lc_prf("argv ERR(argv[1] %s)\r\n", argv[1]);
	}
	
	app_homekit_conf_save(argc, argv);
	/* Applications/Main.c main() */
	main(argc, argv);
}

/* register homekit CLI */
static const struct cli_command homekit_clis[] =
{
	{"homekit", "homekit sta/ap ssid key(null if open)", hk_command},
};

int app_homekit_cli_register(void)
{
	int ret;

	ret = cli_register_commands(homekit_clis, sizeof(homekit_clis)/sizeof(struct cli_command));
	if (ret) {
		lc_prf("cli_register_commands() ret %d\r\n", ret);
	}

	return ret;
}

void app_wifi_init(void)
{
// void entry_main(void) <arch_main.c> will called from beken378/driver/entry/boot_handlers.S
	lc_prf("%s-%d Do nothing\r\n", __func__, __LINE__);
}

int app_wifi_connect(void)
{
	return bk_wlan_start(&wNetConfig);
}

int app_wifi_softap_start(void)
{
	int ret = -1;

	//assert(wNetConfig.wifi_mode == BK_SOFT_AP);
	ret = bk_wlan_start(&wNetConfig);
	if (ret) {
		lc_prf("bk_wlan_start() ret %d", ret);
	}

	return ret;
}

/* SoftAP add the given IE as part of beacon and probe response frames.
 * custom IE can be add by vendor_elements=xx in hostapd.conf
 * Apple Device IE elements are defined in
 * Apple Device Information Element section of the Accessory Interface Specification- Wi-Fi Accessory Configuration Addendum R1
 */
int app_wifi_softap_add_ie(const void* ieBytes, size_t numIEBytes)
{
	//CALL wpa_config_process_ap_vendor_elements()
	//to refer to branch SDK_3.0.15_midea of beken_wifi_sdk.git Commit b828918a
	// Added BCN Vendor-IE resource protection and updated BCN data interface
	int ret = -1;
	ret = set_beacon_ie_vendor_elements_addition((unsigned char *)ieBytes, numIEBytes);
	if (ret) {
		lc_prf("set_beacon_ie_vendor_elements_addition() ret %d", ret);
		return ret;
	}

	ret = ieee802_11_update_beacons_params();
	if (ret) {
		lc_prf("ieee802_11_update_beacons_params() ret %d", ret);
	}

	return ret;
	
}

int app_wifi_softap_stop(void)
{
	int ret = -1;

	if (wNetConfig.wifi_mode == BK_SOFT_AP) {
		set_beacon_ie_vendor_elements_clear();
		ret = bk_wlan_stop(BK_SOFT_AP);
		if (ret) {
			lc_prf("bk_wlan_stop() ret %d", ret);
		}

		return ret;
	} else {
		return 0;
	}
}
