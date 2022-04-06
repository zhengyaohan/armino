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

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	QVGA_320_240    = 0,
	VGA_640_480,
	VGA_800_600,
	VGA_1280_720,
	VGA_480_272,
	PPI_MAX
} PPI_TYPE; // Pixel per inch

typedef enum {
	TYPE_5FPS            = 0,
	TYPE_10FPS,
	TYPE_15FPS,
	TYPE_20FPS,
	TYPE_25FPS,
	TYPE_30FPS,
	FPS_MAX
} FPS_TYPE; // frame per second

#define PPI_POSI        0
#define PPI_MASK        0xFF
#define FPS_POSI        8
#define FPS_MASK        0xFF

#define CMPARAM_SET_PPI(p, x)   (p = ((p & ~(PPI_MASK << PPI_POSI)) | ((x & PPI_MASK) << PPI_POSI)))
#define CMPARAM_GET_PPI(p)      ((p >> PPI_POSI) & PPI_MASK)

#define CMPARAM_SET_FPS(p, x)   (p = ((p & ~(FPS_MASK << FPS_POSI)) | ((x & FPS_MASK) << FPS_POSI)))
#define CMPARAM_GET_FPS(p)      ((p >> FPS_POSI) & FPS_MASK)

typedef enum {
	TVIDEO_OPEN_NONE         = 0LU,
	TVIDEO_OPEN_SCCB,
	TVIDEO_OPEN_SPIDMA,
} TVIDEO_OPEN_TYPE;

typedef enum {
	TVIDEO_SND_NONE         = 0LU,
	TVIDEO_SND_UDP,
	TVIDEO_SND_TCP,
	TVIDEO_SND_INTF,
	TVIDEO_SND_BUFFER,
} TVIDEO_SND_TYPE;

typedef struct tvideo_desc {
	UINT8 *rxbuf;

	void (*node_full_handler)(void *curptr, UINT32 newlen, UINT32 is_eof, UINT32 frame_len);
	void (*data_end_handler)(void);

	UINT16 rxbuf_len;
	UINT16 rx_read_len;
	UINT32 node_len;
	UINT32 sener_cfg;
} TVIDEO_DESC_ST, *TVIDEO_DESC_PTR;

typedef struct tvideo_hdr_param {
	UINT8 *ptk_ptr;
	UINT32 ptklen;
	UINT32 frame_id;
	UINT32 is_eof;
	UINT32 frame_len;
} TV_HDR_PARAM_ST, *TV_HDR_PARAM_PTR;

typedef void (*tvideo_add_pkt_header)(TV_HDR_PARAM_PTR param);
typedef int (*video_transfer_send_func)(UINT8 *data, UINT32 len);
typedef void (*video_transfer_start_cb)(void);
typedef void (*video_transfer_end_cb)(void);

typedef struct tvideo_setup_desc {
	UINT16 open_type;
	UINT16 send_type;
	video_transfer_send_func send_func;
	video_transfer_start_cb start_cb;
	video_transfer_start_cb end_cb;

	UINT32 pkt_header_size;
	tvideo_add_pkt_header add_pkt_header;
} TVIDEO_SETUP_DESC_ST, *TVIDEO_SETUP_DESC_PTR;

#ifdef __cplusplus
}
#endif