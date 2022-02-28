#include "include.h"
#include "dd_pub.h"
#include "bk_drv_model.h"
#include "bk_pm_model.h"
#include "bk_sys_ctrl.h"
#include "sys_driver.h"

#if ((!CONFIG_SOC_BK7231) && (CONFIG_SECURITY == 1))
#include "bk_security.h"
#endif

#if CONFIG_SOC_BK7256XX  || CONFIG_SOC_BK7256_CP1
#include "aon_pmu_driver.h"
#endif

#if (!CONFIG_RTT) && (!CONFIG_SOC_BK7256_CP1)
#include "BkDriverFlash.h"
#endif

#if CONFIG_SDCARD_HOST
#include "sdcard.h"
#endif

#include "bk_api_int.h"
#include "bk_api_pwm.h"
#include "bk_api_timer.h"
#include "bk_api_gpio.h"
#include "bk_api_dma.h"
#include "bk_api_uart.h"
#include "bk_api_wdt.h"
#include "bk_api_trng.h"
#include "bk_api_efuse.h"
#include "bk_api_mem.h"
#include "bk_api_adc.h"
#include "bk_api_spi.h"
#include "bk_api_i2c.h"
#include "bk_api_aon_rtc.h"

#if CONFIG_QSPI
#include "bk_api_qspi.h"
#endif

#if CONFIG_PSRAM
#include "bk_api_psram.h"
#endif

#if CONFIG_JPEG
#include "bk_api_jpeg.h"
#endif

#include "bk_private/bk_driver.h"
#include "interrupt_base.h"

#if CONFIG_ATE
#include "bk_api_ate.h"
#endif

#if CONFIG_USB
#include "bk_fusb.h"
#include "bk_usb.h"
#endif


//TODO only init driver model and necessary drivers
#if CONFIG_SOC_BK7256XX
//#define   MODULES_POWER_OFF_ENABLE (1)
extern void clock_dco_cali(UINT32 speed);
void power_clk_rf_init()
{
    uint32_t param =0;
	/*power on all the modules for bringup test*/
    
	module_name_t use_module = MODULE_NAME_WIFI;
	dev_clk_pwr_id_t devid = 0;
    /*1. power on all the modules*/
	#if MODULES_POWER_OFF_ENABLE
	    sys_drv_module_power_ctrl(POWER_MODULE_NAME_ENCP,POWER_MODULE_STATE_OFF);
		//sys_drv_module_power_ctrl(POWER_MODULE_NAME_BAKP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_AUDP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_BTSP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_WIFIP_MAC,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_WIFI_PHY,POWER_MODULE_STATE_OFF);
		//sys_drv_module_power_ctrl(POWER_MODULE_NAME_CPU1,POWER_MODULE_STATE_OFF);
	#else
	    power_module_name_t module = POWER_MODULE_NAME_MEM1;
        for(module = POWER_MODULE_NAME_MEM1 ; module < POWER_MODULE_NAME_NONE ; module++)
        {
            sys_drv_module_power_ctrl(module,POWER_MODULE_STATE_ON);
        }
    #endif
    /*2. enable the analog clock*/
    sys_drv_module_RF_power_ctrl(use_module ,POWER_MODULE_STATE_ON);

	/*3.enable all the modules clock*/
	for(devid = 0; devid < 32; devid++)
	{
	    sys_drv_dev_clk_pwr_up(devid, CLK_PWR_CTRL_PWR_UP);
    }
	
    /*4.set the cpu0 and matrix clock*/
   /*cpu0:26m ,matrix:26m*/
   //sys_drv_core_bus_clock_ctrl(HIGH_FREQUECY_CLOCK_MODULE_CPU0, 1,0, HIGH_FREQUECY_CLOCK_MODULE_CPU0_MATRIX,0,0);
   /*cpu0:120m ,matrix:120m*/
   sys_drv_core_bus_clock_ctrl(HIGH_FREQUECY_CLOCK_MODULE_CPU0, 3,0, HIGH_FREQUECY_CLOCK_MODULE_CPU0_MATRIX,0,0);

   /*5.config the analog*/
   //sys_drv_analog_set(ANALOG_REG0, param);
   //sys_drv_analog_set(ANALOG_REG0, param);
   //sys_drv_analog_set(ANALOG_REG0, param);

   	/*set low power low voltage value */
	param = sys_drv_analog_get(ANALOG_REG2);
	param |= (0x1 << 25);
	sys_drv_analog_set(ANALOG_REG2, param);

	param = 0;
	param = sys_drv_analog_get(ANALOG_REG3);
	param &= ~(0x3f << 26);
	param |= (0x1 << 26);
	param |= (0x4 << 29);
	sys_drv_analog_set(ANALOG_REG3, param);

	//config apll
	//param = 0;
	//param = sys_drv_analog_get(ANALOG_REG4);
	//param &= ~(0x1f << 5);
	//param |= (0x14 << 5);
	//sys_drv_analog_set(ANALOG_REG4, param);

	/*tempreture det enable for VIO*/
	param = 0;
	param = sys_drv_analog_get(ANALOG_REG6);
	param |= (0x1 << 10);
	sys_drv_analog_set(ANALOG_REG6, param);

    /*select lowpower lpo clk source*/
    param = 0;
    param = aon_pmu_drv_reg_get(PMU_REG0x41);
    param &= ~0x3; //select clk_DIVD as lpo_src
	aon_pmu_drv_reg_set(PMU_REG0x41,param);

   /*7.dpll calibration */
   //sys_drv_cali_dpll(0);

   /*8.dco calibration*/
   //clock_dco_cali(0x4);
}
#endif
int driver_init(void)
{
	sys_drv_init();
#if CONFIG_SOC_BK7256XX
    aon_pmu_drv_init();
    power_clk_rf_init();
#endif
	interrupt_init();

	bk_gpio_driver_init();

	//Important notice!!!!!
	//ATE uses UART TX PIN as the detect ATE mode pin,
	//so it should be called after GPIO init and before UART init.
	//or caused ATE can't work or UART can't work
#if CONFIG_ATE
	bk_ate_init();
#endif

	//Important notice!
	//Before UART is initialized, any call of bk_printf/os_print/BK_LOGx may
	//cause problems, such as crash etc!

	bk_uart_driver_init();

	os_show_memory_config_info(); //TODO - remove it after bk_early_printf() is supported.
	drv_model_init();

	g_dd_init();

#if (!CONFIG_SOC_BK7256_CP1)
#if CONFIG_SOC_BK7256XX// || CONFIG_SOC_BK7256_CP1
	//dev_pm_init();
	extern void rf_ps_pm_init(void);
	rf_ps_pm_init();
#else
	dev_pm_init();
#endif
#endif

#if ((!CONFIG_SOC_BK7231) && (CONFIG_SECURITY == 1))
	bk_secrity_init();
#endif

#if (!CONFIG_RTT) && (!CONFIG_SOC_BK7256_CP1)
	hal_flash_init();
#endif

#if  (!CONFIG_SOC_BK7256_CP1)
	bk_pwm_driver_init();
#endif

	bk_timer_driver_init();
	bk_dma_driver_init();
	bk_wdt_driver_init();

#if  (!CONFIG_SOC_BK7256_CP1)
	bk_trng_driver_init();
	bk_efuse_driver_init();
	bk_adc_driver_init();
#endif
	bk_spi_driver_init();
	bk_i2c_driver_init();

#if CONFIG_QSPI
	bk_qspi_driver_init();
#endif

#if CONFIG_JPEG
	bk_jpeg_driver_init();
#endif

#if CONFIG_PSRAM
	bk_psram_driver_init();
#endif

#if CONFIG_AONT_RTC
	bk_aon_rtc_driver_init();
#endif

#if CONFIG_SDCARD_HOST
	sdcard_init();
#endif

//call it after LOG is valid.
#if CONFIG_ATE
	os_printf("ate enabled is %d\r\n", ate_is_enabled());
#endif

#if CONFIG_USB
   usb_init();
   fusb_init();
#endif

   os_printf("driver_init end\r\n");

	return 0;
}

// eof
