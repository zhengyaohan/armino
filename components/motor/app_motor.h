#ifndef __APP_MOTOR_H__
#define __APP_MOTOR_H__

typedef enum {
	FLIP_MIRROR = 0,
	FLIP_UPDN,
	FLIP_ALL,
	FLIP_MAX
} FLIP_TYPE;



#define	UPDN_MOTOR	MOTOR1
#define	RL_MOTOR		MOTOR2

#define	STEP_ANGLE_RL			15			//30
#define	STEP_ANGLE_UPDN		15			//30

#define	CMD_PTZ_UP 				0
#define	CMD_PTZ_UP_STOP 			1
#define	CMD_PTZ_DOWN 				2
#define	CMD_PTZ_DOWN_STOP 		3
#define	CMD_PTZ_LEFT 				4
#define	CMD_PTZ_LEFT_STOP 		5
#define	CMD_PTZ_RIGHT 				6
#define	CMD_PTZ_RIGHT_STOP 		7
#define	CMD_PTZ_H_ROTATE 			8
#define	CMD_PTZ_H_ROTATE_STOP	9
#define	CMD_PTZ_V_ROTATE 			10
#define	CMD_PTZ_V_ROTATE_STOP 	11

#define	CMD_CAMERA_HID 			12
#define	CMD_CAMERA_FHID 			13

#define	CMD_PTZ_LED 				14
#define	CMD_PTZ_IR_LED 			15
#define	CMD_PTZ_SOFT_VERSION		16


int App_Motor_Ctl(char *output, int cmd, int para);

#endif

