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

#include <arch/soc.h>
#include "driver/jpeg_dec_types.h"



#ifdef __cplusplus
extern "C" {
#endif

#define addJPEG_data_st                                         (addJPEG_Reg0x9 & 0x100)
#define addJPEG_ycount                                          (addJPEG_Reg0x9 & 0xff)
#define WORK_AREA_SIZE 4096

//volatile uint32_t* rd_buff = (uint32_t *)0x30020000;//0x60000000; //lea add

//Test Handle



#define	JD_SZBUF		1024	/* Size of stream input buffer */
#define JD_FORMAT		0	/* Output pixel format 0:RGB888 (3 BYTE/pix), 1:RGB565 (1 WORD/pix) */
#define	JD_USE_SCALE	1	/* Use descaling feature for output */
#define JD_TBLCLIP		1	/* Use table for saturation (might be a bit faster but increases 1K bytes of code size) */

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif



//static 	JDEC jdec; /* Decompression object */


/* TJpgDec API functions */
JRESULT jd_prepare (JDEC*, uint16_t(*)(JDEC*,uint8_t*,uint16_t), void*, uint16_t, void*);

/* JDEC :Initialized decompression object */
JRESULT jd_decomp (JDEC* jd, uint8_t size, uint32_t* dec_src_addr, uint32_t* dec_dest_addr);


void jpeg_hal_dec_init(void);
void bk_jpeg_dec_isr_register(int_isr_t jpeg_dec_isr);

void bk_jpeg_dec_isr_unregister(void);
void jpegenc_en(void);

void jpegenc_off(void);

void JpegdecInit(JDEC* jdec,  uint32_t * dec_src_addr);

int jpg_decoder_init(void);


#ifdef __cplusplus
}
#endif









