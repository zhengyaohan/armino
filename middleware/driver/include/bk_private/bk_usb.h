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

#ifdef __cplusplus
extern "C" {
#endif

#include "include.h"

#define UVC_DEMO_SUPPORT102

#define USB_FAILURE                (1)
#define USB_SUCCESS                (0)

#define USB_DEV_NAME                "usb"

typedef void (*USB_FPTR)(void *, void *);

#define USB_CMD_MAGIC              (0xe550000)
enum
{
    UCMD_RESET = USB_CMD_MAGIC + 1,

    UCMD_MSC_REGISTER_FIDDLE_CB,
    UCMD_UVC_REGISTER_CONFIG_NOTIFY_CB,
    UCMD_UVC_REGISTER_RX_VSTREAM_CB,
    UCMD_UVC_REGISTER_RX_VSTREAM_BUF_PTR,
    UCMD_UVC_REGISTER_RX_VSTREAM_BUF_LEN,

    UCMD_UVC_SET_PARAM,
    UCMD_UVC_START_STREAM,
    UCMD_UVC_STOP_STREAM,
    UCMD_UVC_GET_CONNECT_STATUS,
    UCMD_UVC_RECEIVE_VSTREAM,
    UCMD_UVC_ENABLE_MJPEG,
    UCMD_UVC_ENABLE_H264,

	UCMD_USB_CONNECTED_REGISTER_CB
};

/*UCMD_UVC_SET_PARAM*/
#define UVC_MUX_PARAM(resolution_id, fps)           (fps + (resolution_id << 16))
#define UVC_DEMUX_FPS(param)                         (param & 0xffff)
#define UVC_DEMUX_ID(param)                         ((param >> 16) & 0xffff)

typedef enum 
{
    USB_HOST_MODE   = 0,
    USB_DEVICE_MODE = 1
} USB_MODE;

/*
 * The value is defined in field wWidth and wHeight in 'Video Streaming MJPEG
Frame Type Descriptor'
 */
#ifdef UVC_DEMO_SUPPORT100
typedef enum
{
    U2_FRAME_640_480 = 1,
    U2_FRAME_640_360 = 2,
    U2_FRAME_320_240 = 3,
    U2_FRAME_168_120 = 4,
} E_FRAME_ID_USB20;

typedef enum
{
    U1_FRAME_640_480 = 1,
    U1_FRAME_160_120 = 2,
} E_FRAME_ID_USB11;
#elif defined(UVC_DEMO_SUPPORT102)
typedef enum
{
    UVC_FRAME_352_288 = 0,
    UVC_FRAME_320_240 = 1,
    UVC_FRAME_640_360 = 2,
    UVC_FRAME_640_480 = 3,
    UVC_FRAME_COUNT
} E_FRAME_ID_USB20;
#endif

typedef enum
{
    FPS_30 = 30,
    FPS_25 = 25,
    FPS_20 = 20,
    FPS_15 = 15,
    FPS_10 = 10,
    FPS_5  = 5,
} E_FRAME_RATE_ID;
	
/*
* Finish DRC interrupt processing
*/
enum
{
	BSR_NONE_EVENT = 0,
	BSR_ERROR_EVENT,
	BSR_CONNECT_EVENT,
	BSR_CONNECTED_EVENT,
	
	BSR_DISCONNECT_EVENT,
	BSR_READ_OK_EVENT,
    BSR_WRITE_OK_EVENT
};

/*******************************************************************************
* Function Declarations
*******************************************************************************/
extern void usb_init(void);
extern void usb_exit(void);
extern uint32_t MUSB_HfiRead( uint32_t first_block, uint32_t block_num, uint8_t
                              *dest);
extern uint32_t MUSB_HfiWrite( uint32_t first_block, uint32_t block_num, uint8_t
                               *dest);
extern void MGC_RegisterCBTransferComplete(FUNCPTR func);
extern uint8_t MUSB_GetConnect_Flag(void);

#if (CONFIG_SOC_BK7251) || (CONFIG_SOC_BK7271)
#define USB_PLUG_FAILURE                (1)
#define USB_PLUG_SUCCESS                (0)

#define USB_PLUG_DEV_NAME               "usb_plug"
#include "bk_gpio.h"

void usb_plug_inout_init(void);
void usb_plug_inout_exit(void);
#endif // (CONFIG_SOC_BK7251)

#ifdef __cplusplus
}
#endif
