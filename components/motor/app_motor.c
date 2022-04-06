#include <common/bk_include.h>

#include "FreeRTOS.h"
#include "task.h"
#include <os/os.h>
#include <common/bk_kernel_err.h>
#include "bk_fake_clock.h"
#include <os/mem.h>
#include "bk_gpio.h"
#include "app_led.h"
#include "app_demo_apcfg.h"
#include "app_demo_softap.h"
#include "bk_wifi_private.h"
#include "bk_rw.h"
#include "common.h"

#include <components/camera_intf_pub.h>
#include "motor_main.h"
#include "app_motor.h"
#include <components/video_transfer.h>


#include <tzh_ppcs_api.h>
#include <cgi_cmd.h>

extern  char *http_ota_get_fw_vision(void);
#ifdef CONFIG_IR_LED
extern void Toggle_Led(void);

#define IR_LED_GPIO_INDEX              GPIO6
#define IR_LED_ON_VAL                  	1         						 // high   LED ON
#define IR_LED_OFF_VAL                 	((~IR_LED_ON_VAL) & 0x1)         	 // low    LED Off
#define IR_LED_INITIAL_VAL             	IR_LED_OFF_VAL

typedef struct IRled_st {
	beken_timer_t led_timer;
	DEV_STATE state;
	GPIO_INDEX gpio_idx;
} IRLED_ST, IRLED_PTR;

static IRLED_ST IRledctr;

void IR_led_init_gpio(void)
{
	IRledctr.gpio_idx = IR_LED_GPIO_INDEX;
	IRledctr.state = LED_OFF;
	bk_gpio_config_output(IRledctr.gpio_idx);
	bk_gpio_output(IRledctr.gpio_idx, IR_LED_INITIAL_VAL);

}

void IR_led_ctl(UINT8 on)
{
	bk_gpio_config_output(IR_LED_GPIO_INDEX);

	if (on) {
		bk_gpio_output(IR_LED_GPIO_INDEX, IR_LED_ON_VAL);
		IRledctr.state = POWER_ON;
	} else {
		bk_gpio_output(IR_LED_GPIO_INDEX, IR_LED_OFF_VAL);
		IRledctr.state = LED_OFF;

	}
}
#endif

int App_Motor_Ctl(char *output, int cmd, int para)
{
	UINT32 m, n;
	int	ret = 0;
	switch (cmd) {
#ifdef CONFIG_PWM_MOTOR
	case CMD_PTZ_UP:
		Motor_Ctrl_Angle(UPDN_MOTOR, REV_DIR, STEP_ANGLE_UPDN, SLOW_SPEED3);

		break;
	case CMD_PTZ_DOWN:
		Motor_Ctrl_Angle(UPDN_MOTOR, NORM_DIR, STEP_ANGLE_UPDN, SLOW_SPEED3);

		break;
	case CMD_PTZ_LEFT:
		Motor_Ctrl_Angle(RL_MOTOR, REV_DIR, STEP_ANGLE_RL, SLOW_SPEED3);
		break;
	case CMD_PTZ_RIGHT:
		Motor_Ctrl_Angle(RL_MOTOR, NORM_DIR, STEP_ANGLE_RL, SLOW_SPEED3);
		break;
#endif
	case CMD_PTZ_H_ROTATE:
		camera_flip(FLIP_MIRROR);
		break;
	case CMD_PTZ_V_ROTATE:
		camera_flip(FLIP_UPDN);
		break;

	case CMD_CAMERA_HID:
		os_printf("CAMERA HID SET!!\r\n");
		m = QVGA_320_240;
		n = TYPE_10FPS;
		video_transfer_set_video_param(m, n);
		break;
	case CMD_CAMERA_FHID:
		os_printf("CAMERA F-HID SET!!\r\n");
		m = VGA_640_480;
		n = TYPE_10FPS;
		video_transfer_set_video_param(m, n);

		break;

	case CMD_PTZ_LED:
		os_printf("CMD_PTZ_LED!!\r\n");
		Toggle_Led();

		break;
#ifdef CONFIG_IR_LED
	case CMD_PTZ_IR_LED:
		if (IRledctr.state == LED_OFF) {
			IR_led_ctl(1);
			camera_intfer_set_BW(COLOUR_BW);
			os_printf("IR led on!!\r\n");

		} else {
			IR_led_ctl(0);
			camera_intfer_set_BW(COLOUR_NORMAL);
			os_printf("IR led off!!\r\n");

		}

		break;
#endif
	case CMD_PTZ_SOFT_VERSION:
		//os_printf("CMD_PTZ_SOFT_VERSION=%s\r\n",http_ota_get_fw_vision());
		//ret += UTILS_WriteStr(output+ret,"sysver",http_ota_get_fw_vision());
		//os_printf("output=:%s\n",output);


		break;

	default:
		break;

	}
	return ret;

}

