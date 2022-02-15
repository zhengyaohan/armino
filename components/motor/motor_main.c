#include "include.h"
#include "bk_api_rtos.h"
#include "BkDriverPwm.h"
#include "bk_drv_model.h"
#include "pwm.h"
#include "bk_kernel_err.h"
#include "bk_pwm.h"
#include <stdint.h>
#include <stdlib.h>
#include "finsh.h"
#include "bk_gpio.h"
#include "motor_main.h"
#include "bk_uart.h"

#define	NEW_MOTOR_PIN_CTL
#ifdef CONFIG_PWM_MOTOR
/*
#define PWM_CHANNEL0_P6 0
#define PWM_CHANNEL1_P7 1
#define PWM_CHANNEL2_P8 2
#define PWM_CHANNEL3_P9 3
#define PWM_CHANNEL4_P24 4
#define PWM_CHANNEL5_P26 5
*/
#define MOTOR_DEBUG
#ifdef MOTOR_DEBUG
#define MOTOR_PRT      os_printf
#define MOTOR_WARN   warning_prf
#define MOTOR_FATAL   fatal_prf
#else
#define MOTOR_PRT      null_prf
#define MOTOR_WARN   null_prf
#define MOTOR_FATAL   null_prf
#endif

#define	MOTOR_PWM_CLK		26										// 26M
#define	MOTOR_PWM_1UNIT		(1000/MOTOR_PWM_CLK)					// 1000/26M=38ns

#define 	MOTOR_PWM_CHANNE 	4


#define	MOTOR_DECELERATE		64										//减速比
#define	MOTOR_STEP_ANGLE		64										//步距角度(360/5.625=64)
#define	MOTOR_1CYCLE_PULSE	((MOTOR_DECELERATE*MOTOR_STEP_ANGLE)/2)	//64*64/2=4096/2=2048 (步长=4)


#define	MOTOR_PULSE_FREQ		650		//HZ
#define	MOTOR_PULSE_DUR		(1000000/MOTOR_PULSE_FREQ)		// 1538us	
#define	MOTOR_1CYCLE_DUR		(MOTOR_DECELERATE*MOTOR_STEP_ANGLE)*MOTOR_PULSE_DUR	// 2048*1.538ms=3.1s


#define MOTOR_PWM_END_DEF 	((MOTOR_PULSE_DUR*1000)/MOTOR_PWM_1UNIT)	// (1538000 /38)
#define MOTOR_PWM_DUTY_DEF 	(MOTOR_PWM_END_DEF/2)

#ifdef NEW_MOTOR_PIN_CTL
#define	MOTOR1_GPIO_INDEX1		GPIO23
#define	MOTOR1_GPIO_INDEX2		GPIO22
#define	MOTOR1_GPIO_INDEX3		GPIO5
#else
#define	MOTOR1_GPIO_INDEX1		GPIO19
#define	MOTOR1_GPIO_INDEX2		GPIO15
#define	MOTOR1_GPIO_INDEX3		GPIO18
#endif
#define	MOTOR1_GPIO_INDEX4		GPIO24

#define	MOTOR2_GPIO_INDEX1		GPIO26
#define	MOTOR2_GPIO_INDEX2		GPIO2
#define	MOTOR2_GPIO_INDEX3		GPIO6
#define	MOTOR2_GPIO_INDEX4		GPIO7

#define TBL_STEP		4

static pwm_param_t motor_pwm;//test_pwm[CHANNEL_NO];
static UINT8 print_cnt = 0; //[CHANNEL_NO]={0};
typedef struct MotorPwm_st {
	UINT16	mtcnt;
	UINT16 motorspeedcnt;
	UINT16 motorpulsecnt;
	UINT8 dir;
	UINT8 enable;
	UINT8 step;

} MOTORPWM_ST, MOTOPWM_PTR;

static MOTORPWM_ST motor1ctr;
static MOTORPWM_ST motor2ctr;

/******************************************************************/
// 1pulse:AB--BC--CD--DA ( forward-rotating)
static const unsigned char MOTOR1_FWD_TBL[TBL_STEP][4] = {
	//   A,B, C, D
#if (TBL_STEP==4)
	{ 0, 1, 1, 0},
	{ 0, 0, 1, 1},
	{ 1, 0, 0, 1},
	{ 1, 1, 0, 0},
#else

	{ 1, 0, 0, 0},
	{ 1, 1, 0, 0},
	{ 0, 1, 0, 0},
	{ 0, 1, 1, 0},

	{ 0, 0, 1, 0},
	{ 0, 0, 1, 1},
	{ 0, 0, 0, 1},
	{ 1, 0, 0, 1},
#endif

};
// 1pulse:AB--AD--CD--CB ( Reversal)
static const unsigned char MOTOR1_REV_TBL[TBL_STEP][4] = {
	//   A,B, C, D
#if (TBL_STEP==4)
	{ 1, 1, 0, 0},
	{ 1, 0, 0, 1},
	{ 0, 0, 1, 1},
	{ 0, 1, 1, 0},

#else

	{ 1, 0, 0, 1},
	{ 0, 0, 0, 1},
	{ 0, 0, 1, 1},
	{ 0, 0, 1, 0},

	{ 0, 1, 1, 0},
	{ 0, 1, 0, 0},
	{ 1, 1, 0, 0},
	{ 1, 0, 0, 0},
#endif
};
// 1pulse:AB--BC--CD--DA ( forward-rotating)
static const unsigned char MOTOR2_FWD_TBL[TBL_STEP][4] = {
	//   A,B, C, D
#if (TBL_STEP==4)
	{ 0, 1, 1, 0},
	{ 0, 0, 1, 1},
	{ 1, 0, 0, 1},
	{ 1, 1, 0, 0},

#else
	{ 1, 0, 0, 0},
	{ 1, 1, 0, 0},
	{ 0, 1, 0, 0},
	{ 0, 1, 1, 0},

	{ 0, 0, 1, 0},
	{ 0, 0, 1, 1},
	{ 0, 0, 0, 1},
	{ 1, 0, 0, 1},
#endif



};
// 1pulse:AB--AD--CD--CB ( Reversal)
static const unsigned char MOTOR2_REV_TBL[TBL_STEP][4] = {
	//   A,B, C, D
#if (TBL_STEP==4)
	{ 1, 1, 0, 0},
	{ 1, 0, 0, 1},
	{ 0, 0, 1, 1},
	{ 0, 1, 1, 0},

#else


	{ 1, 0, 0, 1},
	{ 0, 0, 0, 1},
	{ 0, 0, 1, 1},
	{ 0, 0, 1, 0},

	{ 0, 1, 1, 0},
	{ 0, 1, 0, 0},
	{ 1, 1, 0, 0},
	{ 1, 0, 0, 0},
#endif
};

void reset_motor1_ctlpin(void)
{
	bk_gpio_config_output(MOTOR1_GPIO_INDEX1);
	bk_gpio_config_output(MOTOR1_GPIO_INDEX2);
	bk_gpio_config_output(MOTOR1_GPIO_INDEX3);
	bk_gpio_config_output(MOTOR1_GPIO_INDEX4);

	bk_gpio_output(MOTOR1_GPIO_INDEX1, 0);
	bk_gpio_output(MOTOR1_GPIO_INDEX2, 0);
	bk_gpio_output(MOTOR1_GPIO_INDEX3, 0);
	bk_gpio_output(MOTOR1_GPIO_INDEX4, 0);
}
void reset_motor2_ctlpin(void)
{

	bk_gpio_config_output(MOTOR2_GPIO_INDEX1);
	bk_gpio_config_output(MOTOR2_GPIO_INDEX2);
	bk_gpio_config_output(MOTOR2_GPIO_INDEX3);
	bk_gpio_config_output(MOTOR2_GPIO_INDEX4);

	bk_gpio_output(MOTOR2_GPIO_INDEX1, 0);
	bk_gpio_output(MOTOR2_GPIO_INDEX2, 0);
	bk_gpio_output(MOTOR2_GPIO_INDEX3, 0);
	bk_gpio_output(MOTOR2_GPIO_INDEX4, 0);

}

static void motor_pwm_isr(unsigned char param)
{
	UINT8 i, dt;
	if (motor1ctr.enable)motor1ctr.mtcnt++;
	if ((motor1ctr.mtcnt >= motor1ctr.motorspeedcnt) && (motor1ctr.enable) && (motor1ctr.motorpulsecnt)) {
		motor1ctr.mtcnt = 0;
		i = motor1ctr.step;
		if (motor1ctr.dir == 0) {
			bk_gpio_output(MOTOR1_GPIO_INDEX1, MOTOR1_FWD_TBL[i][0]);
			bk_gpio_output(MOTOR1_GPIO_INDEX2, MOTOR1_FWD_TBL[i][1]);
			bk_gpio_output(MOTOR1_GPIO_INDEX3, MOTOR1_FWD_TBL[i][2]);
			bk_gpio_output(MOTOR1_GPIO_INDEX4, MOTOR1_FWD_TBL[i][3]);
		} else {
			bk_gpio_output(MOTOR1_GPIO_INDEX1, MOTOR1_REV_TBL[i][0]);
			bk_gpio_output(MOTOR1_GPIO_INDEX2, MOTOR1_REV_TBL[i][1]);
			bk_gpio_output(MOTOR1_GPIO_INDEX3, MOTOR1_REV_TBL[i][2]);
			bk_gpio_output(MOTOR1_GPIO_INDEX4, MOTOR1_REV_TBL[i][3]);
		}
		motor1ctr.step++;
		if (motor1ctr.step >= TBL_STEP)motor1ctr.step = 0;
		motor1ctr.motorpulsecnt--;
		if (motor1ctr.motorpulsecnt == 0)
			reset_motor1_ctlpin();


	}
	if (motor2ctr.enable)motor2ctr.mtcnt++;
	if ((motor2ctr.mtcnt >= motor2ctr.motorspeedcnt) && (motor2ctr.enable) && (motor2ctr.motorpulsecnt)) {

		motor2ctr.mtcnt = 0;
		i = motor2ctr.step;
		if (motor2ctr.dir == 0) {
			bk_gpio_output(MOTOR2_GPIO_INDEX1, MOTOR2_FWD_TBL[i][0]);
			bk_gpio_output(MOTOR2_GPIO_INDEX2, MOTOR2_FWD_TBL[i][1]);
			bk_gpio_output(MOTOR2_GPIO_INDEX3, MOTOR2_FWD_TBL[i][2]);
			bk_gpio_output(MOTOR2_GPIO_INDEX4, MOTOR2_FWD_TBL[i][3]);
		} else {
			bk_gpio_output(MOTOR2_GPIO_INDEX1, MOTOR2_REV_TBL[i][0]);
			bk_gpio_output(MOTOR2_GPIO_INDEX2, MOTOR2_REV_TBL[i][1]);
			bk_gpio_output(MOTOR2_GPIO_INDEX3, MOTOR2_REV_TBL[i][2]);
			bk_gpio_output(MOTOR2_GPIO_INDEX4, MOTOR2_REV_TBL[i][3]);
		}
		motor2ctr.step++;
		if (motor2ctr.step >= TBL_STEP)motor2ctr.step = 0;
		motor2ctr.motorpulsecnt--;
		if (motor2ctr.motorpulsecnt == 0)
			reset_motor2_ctlpin();

	}

}
/*
param:	duty & end-- unit is 1/26MHz = 38ns
   |<---------end-------->|
   |<- duty->|            |
    ---------			  |
   |    	 |            |
   |    	 |            |
   |         |            |
	    	  ------------
*/
static void motor_pwm_init(UINT16 channel, UINT8 int_enable, UINT16 duty, UINT16 end)
{
	if (channel > CHANNEL_NO - 1)
		return;

	/*init pwm*/
	motor_pwm.channel = channel;
	motor_pwm.cfg.bits.en	  = PWM_ENABLE;
	motor_pwm.cfg.bits.mode   = PWM_TIMER_MODE;
	motor_pwm.cfg.bits.clk	  = PWM_CLK_26M;

	if (int_enable) {
		motor_pwm.cfg.bits.int_en = PWM_INT_EN;
		motor_pwm.p_Int_Handler = motor_pwm_isr;
	} else {
		motor_pwm.cfg.bits.int_en = PWM_INT_DIS;
		motor_pwm.p_Int_Handler = 0;
	}

	motor_pwm.duty_cycle	  = duty;
	motor_pwm.end_value 	  = end;
	os_printf("Motor pwm init,param:%d,%d,%d,%d\r\n", channel, int_enable, duty, end);
	sddev_control(DD_DEV_TYPE_PWM, CMD_PWM_INIT_PARAM, &motor_pwm);
}

//cmd_motor_pwm_init channel int_enable duty end
static void cmd_motor_pwm_init(int argc, char *argv[])
{
	UINT16 channel, duty, end, int_enable;
	if (argc != 5)
		return;
	channel = atoi(argv[1]);
	int_enable = atoi(argv[2]);
	duty = atoi(argv[3]);
	end = atoi(argv[4]);
	if (channel > CHANNEL_NO - 1)
		return;
	if (end < duty)
		return;
	motor_pwm_init(channel, int_enable, duty, end);
}
MSH_CMD_EXPORT(cmd_motor_pwm_init, cmd pwm init);


//motor_pwm_change_param channel duty value
//motor_pwm_change_param channel endvalue value
static void motor_pwm_change_param(int argc, char *argv[])
{
	uint16 val, channel;
	if (argc != 4)
		return;
	channel = atoi(argv[1]);
	val = atoi(argv[3]);
	if (channel > CHANNEL_NO - 1)
		return;
	if (0 == strcmp(argv[2], "duty"))
		motor_pwm.duty_cycle = val;
	else if (0 == strcmp(argv[2], "endvalue"))
		motor_pwm.end_value = val;
	else
		return;
	sddev_control(DD_DEV_TYPE_PWM, CMD_PWM_INIT_PARAM, &motor_pwm);
}
MSH_CMD_EXPORT(motor_pwm_change_param, pwm change param);

/*********************************************************************************/
void motor_ctl_init(void)
{

	if (motor_pwm.cfg.bits.int_en == PWM_INT_EN) return;

	//motor_pwm_init(MOTOR_PWM_CHANNE,1,20000,40000);
	motor_pwm_init(MOTOR_PWM_CHANNE, 1, MOTOR_PWM_DUTY_DEF, MOTOR_PWM_END_DEF);

}

void motor_ctl_deinit(void)
{

	GLOBAL_INT_DECLARATION();

	GLOBAL_INT_DISABLE();

	motor1ctr.enable = 0;
	motor2ctr.enable = 0;
	motor_pwm_init(MOTOR_PWM_CHANNE, 0, MOTOR_PWM_DUTY_DEF, MOTOR_PWM_END_DEF);
	GLOBAL_INT_RESTORE();


	reset_motor1_ctlpin();
	reset_motor2_ctlpin();
}
void Motor_Ctrl_Angle(MOTOR_INDEXST motorindex, MOTOR_DIRST dir, UINT16 angle, MOTOR_SPEEDST speed)
{
	if (angle == 0)return;
	if ((speed <= 0) || (speed > 5))return;

	if (motorindex == 0) {
		motor1ctr.enable = 0;
		motor1ctr.motorspeedcnt = speed;
		motor1ctr.motorpulsecnt = (((MOTOR_DECELERATE * angle) / 5.625) / 2);		//64*360/5.625/2
		motor1ctr.dir = dir;
		motor1ctr.step = 0;
		motor1ctr.mtcnt = 0;
		reset_motor1_ctlpin();

		motor_ctl_init();
		MOTOR_PRT("motor1 pulsecnt=%d\r\n", motor1ctr.motorpulsecnt);
		motor1ctr.enable = 1;


	} else {
		motor2ctr.enable = 0;
		motor2ctr.motorspeedcnt = speed;
		motor2ctr.motorpulsecnt = (((MOTOR_DECELERATE * angle) / 5.625) / 2);		//64*360/5.625/2
		motor2ctr.dir = dir;
		motor2ctr.step = 0;
		motor2ctr.mtcnt = 0;
		reset_motor2_ctlpin();

		motor_ctl_init();
		MOTOR_PRT("motor2 pulsecnt=%d\r\n", motor2ctr.motorpulsecnt);

		motor2ctr.enable = 1;

	}
}
#if CONFIG_RTT
void motorcmd(int argc, char **argv)
{
	UINT16 len1, len2;
	UINT8 index, dir, speed;
	UINT16 angle;
	index = atoi(argv[1]);
	dir = atoi(argv[2]);
	angle = atoi(argv[3]);
	speed = atoi(argv[4]);
	if ((index < 2) && (speed >= 1) && (angle <= 360))
		Motor_Ctrl_Angle(index, dir, angle, speed);


	else
		os_printf("motorcmd index dir angle speed \r\n");
}
FINSH_FUNCTION_EXPORT_ALIAS(motorcmd, __cmd_motor, motorcmd);

#endif

#endif
