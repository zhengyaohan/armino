#ifndef __MOTOR_MAIN_H__
#define __MOTOR_MAIN_H__
/******************************************************/
/*customer name*/
#define	CUS_SERV_TZH			1
#define	CUS_SERV_JXL			2
#define	CUS_SERV_CY			3
#define	CUS_SERV_XSJ			4

//#define	CUS_SERVER				CUS_SERV_XSJ
#define	CUS_SERVER				CUS_SERV_TZH
/******************************************************/
/*customer camera*/
#define	CUSCAM_GC0328C		0
#define	CUSCAM_GC0309			1


#define	CUS_CAMERA			CUSCAM_GC0328C
/******************************************************/
/*board module*/
#define	TZH_CUS1_MOTOR		1
#define	TZH_CUS2_IRLED		2

#define	TZH_CUS_MODULE		TZH_CUS2_IRLED

#if(TZH_CUS_MODULE==TZH_CUS1_MOTOR)
#define 	CONFIG_PWM_MOTOR
#elif(TZH_CUS_MODULE==TZH_CUS2_IRLED)
#define	CONFIG_IR_LED
#endif
/******************************************************/

typedef enum {
	NORM_DIR         = 0,
	REV_DIR,

} MOTOR_DIRST;

typedef enum {
	MOTOR1         = 0,
	MOTOR2

} MOTOR_INDEXST;

typedef enum {
	NORM_SPEED         = 1,
	SLOW_SPEED2,
	SLOW_SPEED3	,
	SLOW_SPEED4

} MOTOR_SPEEDST;
void motor_ctl_deinit(void);

void Motor_Ctrl_Angle(MOTOR_INDEXST motorindex, MOTOR_DIRST dir, UINT16 angle, MOTOR_SPEEDST speed);
void IR_led_init_gpio(void);


#endif

