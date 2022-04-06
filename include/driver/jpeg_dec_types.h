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

#include <common/bk_err.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*int_isr_t)(void);
//uint32_t* rd_buff = (uint32_t *)0x60000000;//0x30004f00;//; //lea add
//uint32_t* JpegRxBuff = (uint32_t *)0x30000000;

/* Error code */
typedef enum {
	JDR_OK = 0,	/* 0: Succeeded */
	JDR_INTR,	/* 1: Interrupted by output function */
	JDR_INP,	/* 2: Device error or wrong termination of input stream */
	JDR_MEM1,	/* 3: Insufficient memory pool for the image */
	JDR_MEM2,	/* 4: Insufficient stream input buffer */
	JDR_PAR,	/* 5: Parameter error */
	JDR_FMT1,	/* 6: Data format error (may be damaged data) */
	JDR_FMT2,	/* 7: Right format but not supported */
	JDR_FMT3	/* 8: Not supported JPEG standard */
} JRESULT;



/* Rectangular structure */
typedef struct {
	uint16_t left, right, top, bottom;
} JRECT;



/* Decompressor object structure */
typedef struct JDEC JDEC;
struct JDEC {
	uint16_t dctr;				/* Number of bytes available in the input buffer */
	uint8_t* dptr;				/* Current data read ptr */
	uint8_t* inbuf;				/* Bit stream input buffer */
	uint8_t dmsk;				/* Current bit in the current read byte */
	uint8_t scale;				/* Output scaling ratio */
	uint8_t msx, msy;			/* MCU size in unit of block (width, height) */
	uint8_t qtid[3];			/* Quantization table ID of each component */
	int16_t dcv[3];				/* Previous DC element of each component */
	uint16_t nrst;				/* Restart inverval */
	uint16_t width, height;		/* Size of the input image (pixel) */
	uint8_t* huffbits[2][2];	/* Huffman bit distribution tables [id][dcac] */
	uint16_t* huffcode[2][2];	/* Huffman code word tables [id][dcac] */
	uint8_t* huffdata[2][2];	/* Huffman decoded data tables [id][dcac] */
	int32_t* qttbl[4];			/* Dequantizer tables [id] */
	void* workbuf;				/* Working buffer for IDCT and RGB output */
	uint8_t* mcubuf;			/* Working buffer for the MCU */
	void* pool;					/* Pointer to available memory pool */
	uint16_t sz_pool;			/* Size of momory pool (bytes available) */
	uint16_t (*infunc)(JDEC*, uint8_t*, uint16_t);/* Pointer to jpeg stream input function */
	void* device;				/* Pointer to I/O device identifiler for the session */
};


/*
 * @}
 */

#ifdef __cplusplus
}
#endif


