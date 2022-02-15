#include "cli.h"
#include "wlan_ui_pub.h"
#include "bk_manual_ps.h"
#include "bk_mac_ps.h"
#include "bk_mcu_ps.h"
#include "bk_ps.h"
#include "bk_wifi.h"

static void cli_ps_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if PS_SUPPORT_MANUAL_SLEEP
	UINT32 standby_time = 0;
	UINT32 dtim_wait_time = 0;
#endif

	if (argc != 3)
		goto _invalid_ps_arg;

#if CONFIG_MCU_PS
	if (0 == os_strcmp(argv[1], "mcudtim")) {
		UINT32 dtim = os_strtoul(argv[2], NULL, 10);
		if (dtim == 1)
			bk_wlan_mcu_ps_mode_enable();
		else if (dtim == 0)
			bk_wlan_mcu_ps_mode_disable();
		else
			goto _invalid_ps_arg;
	}
#endif
#if CONFIG_STA_PS
	else if (0 == os_strcmp(argv[1], "rfdtim")) {
		UINT32 dtim = os_strtoul(argv[2], NULL, 10);
		if (dtim == 1) {
			if (bk_wlan_dtim_rf_ps_mode_enable())
				os_printf("dtim enable failed\r\n");
		} else if (dtim == 0) {
			if (bk_wlan_dtim_rf_ps_mode_disable())
				os_printf("dtim disable failed\r\n");
		} else
			goto _invalid_ps_arg;
	}
#if PS_USE_KEEP_TIMER
	else if (0 == os_strcmp(argv[1], "rf_timer")) {
		UINT32 dtim = os_strtoul(argv[2], NULL, 10);

		if (dtim == 1) {
			extern int bk_wlan_dtim_rf_ps_timer_start(void);
			bk_wlan_dtim_rf_ps_timer_start();
		} else  if (dtim == 0) {
			extern int bk_wlan_dtim_rf_ps_timer_pause(void);
			bk_wlan_dtim_rf_ps_timer_pause();
		} else
			goto _invalid_ps_arg;
	}
#endif
#endif
	else
		goto _invalid_ps_arg;


	return;

_invalid_ps_arg:
	os_printf("Usage:ps {rfdtim|mcudtim|rf_timer} {1/0}\r\n");
}

static void cli_deep_sleep_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	PS_DEEP_CTRL_PARAM deep_sleep_param;

	deep_sleep_param.wake_up_way            = 0;

	deep_sleep_param.gpio_index_map         = os_strtoul(argv[1], NULL, 16);
	deep_sleep_param.gpio_edge_map          = os_strtoul(argv[2], NULL, 16);
	deep_sleep_param.gpio_last_index_map    = os_strtoul(argv[3], NULL, 16);
	deep_sleep_param.gpio_last_edge_map     = os_strtoul(argv[4], NULL, 16);
	deep_sleep_param.sleep_time             = os_strtoul(argv[5], NULL, 16);
	deep_sleep_param.wake_up_way            = os_strtoul(argv[6], NULL, 16);
	deep_sleep_param.gpio_stay_lo_map       = os_strtoul(argv[7], NULL, 16);
	deep_sleep_param.gpio_stay_hi_map       = os_strtoul(argv[8], NULL, 16);

	if (argc == 9) {
		os_printf("---deep sleep test param : 0x%0X 0x%0X 0x%0X 0x%0X %d %d\r\n",
				  deep_sleep_param.gpio_index_map,
				  deep_sleep_param.gpio_edge_map,
				  deep_sleep_param.gpio_last_index_map,
				  deep_sleep_param.gpio_last_edge_map,
				  deep_sleep_param.sleep_time,
				  deep_sleep_param.wake_up_way);

#if (!CONFIG_SOC_BK7271) && (CONFIG_DEEP_PS)
		bk_enter_deep_sleep_mode(&deep_sleep_param);
#endif
	} else
		os_printf("---argc error!!! \r\n");
}

void cli_mac_ps_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if (CONFIG_MCU_PS | CONFIG_STA_PS)
	UINT32 dtim = 0;
#endif

#if PS_SUPPORT_MANUAL_SLEEP
	UINT32 standby_time = 0;
	UINT32 dtim_wait_time = 0;
#endif

	if (argc < 2)
		goto _invalid_mac_ps_arg;
#if CONFIG_FAKE_RTC_PS
	if (0 == os_strcmp(argv[1], "idleps")) {
		GLOBAL_INT_DECLARATION();
		int count = 0;
		bk_printf("[ARF]rwnxl_reset_evt\r\n");
		HAL_FATAL_ERROR_RECOVER(0); // rwnxl_reset_evt(0);

		rtos_delay_milliseconds(10);

		while (1) {
			GLOBAL_INT_DISABLE();
			evt_field_t field = ke_evt_get();
			GLOBAL_INT_RESTORE();

			if (!(field & KE_EVT_RESET_BIT))
				break;

			rtos_delay_milliseconds(10);
			if (++count > 10000) {
				bk_printf("%s: failed\r\n", __func__);
				break;
			}
		}

		//bk_enable_unconditional_sleep();
		count = 100;
		while ((1 == bk_unconditional_normal_sleep(0xFFFFFFFF, 1))) {
			rtos_delay_milliseconds(2);
			count--;
			if (count == 0) {
				bk_printf("IDLE_SLEEP timeout\r\n");
				break;
			}
		}

		bk_printf("idle Sleep out\r\n");
	}
#endif

#if CONFIG_STA_PS
	else if (0 == os_strcmp(argv[1], "rfwkup")) {
		mac_ps_wakeup_immediately();
		os_printf("done.\r\n");
	} else if (0 == os_strcmp(argv[1], "bcmc")) {
		if (argc != 3)
			goto _invalid_mac_ps_arg;

		dtim = os_strtoul(argv[2], NULL, 10);

		if (dtim == 0 || dtim == 1)
			power_save_sm_set_all_bcmc(dtim);
		else
			goto _invalid_mac_ps_arg;
	} else if (0 == os_strcmp(argv[1], "listen")) {
		if (argc != 4)
			goto _invalid_mac_ps_arg;

		if (0 == os_strcmp(argv[2], "dtim")) {
			dtim = os_strtoul(argv[3], NULL, 10);
			power_save_set_dtim_multi(dtim);

		} else
			goto _invalid_mac_ps_arg;

	} else if (0 == os_strcmp(argv[1], "dump")) {
#if CONFIG_MCU_PS
		mcu_ps_dump();
#endif
		power_save_dump();
	}
#endif
	else
		goto _invalid_mac_ps_arg;

	return;
_invalid_mac_ps_arg:
	os_printf("Usage:mac_ps {func} [param1] [param2]\r\n");
#if CONFIG_FAKE_RTC_PS
	os_printf("mac_ps {idleps}\r\n");
#endif
#if CONFIG_STA_PS
	os_printf("mac_ps {rfwkup}\r\n");
	os_printf("mac_ps {bcmc} {1|0}\r\n");
	os_printf("mac_ps {listen} {dtim} {dtim_val}\r\n");
#endif
	os_printf("mac_ps dump\r\n");
}

void cli_pwr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int pwr = 0;

	if (argc != 3) {
		os_printf("Usage: pwr [hex:5~15].");
		return;
	}

	pwr = os_strtoul(argv[2], NULL, 16);
	if (0 == os_strcmp(argv[1], "sta")) {
		bk_wifi_sta_set_power(pwr);
	} else if (0 == os_strcmp(argv[1], "ap")) {
		bk_wifi_ap_set_power(pwr);
	}
}

#define PWR_CMD_CNT (sizeof(s_pwr_commands) / sizeof(struct cli_command))
static const struct cli_command s_pwr_commands[] = {
	{"ps", "ps {rfdtim|mcudtim|rf_timer} {1|0}", cli_ps_cmd},
	{"mac_ps", "mac_ps {func} [param1] [param2]", cli_mac_ps_cmd},
	{"deep_sleep", "deep_sleep [param]", cli_deep_sleep_cmd},
#if CONFIG_TPC_PA_MAP
	{"pwr", "pwr {sta|ap} pwr", cli_pwr_cmd },
#endif
};

int cli_pwr_init(void)
{
	return cli_register_commands(s_pwr_commands, PWR_CMD_CNT);
}
