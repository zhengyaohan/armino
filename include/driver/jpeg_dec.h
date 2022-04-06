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
//#include <common/bk_include.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief  bk_jpegenc_off
 * @param  none
 * @return none.
 */
void bk_jpegenc_off(void);

/**
 * @brief  bk_jpeg_dec_sys_init
 * @param  none
 * @return none.
 */
void bk_jpeg_dec_sys_init(void);

/**
 * @brief  bk_Jpegdec_config
 * @param1  jdec
 * @param2  xpixel, assign the mem size according x pixel
 * @param3  dec_src_addr decode data src addr
 * @param4  dec_dest_addr decode data dest addr
 * @return none.
 */
void bk_jpegdec_config(JDEC* jdec, uint8_t xpixel, uint32_t* dec_src_addr, uint32_t* dec_dest_addr);

/**
 * @brief  bk_Jpegdec_config
 * @param1 jdec
 * @return none.
 */
	void bk_jpegdec_init(JDEC* jdec, uint32_t * dec_src_addr);

/**
 * @brief  enable JPEG DEC 
 * @param1 NONE
 * @return none.
 */
void bk_jpegenc_en(void);

/**
 * @brief  bk_jpegdec_get_mcuy
 * @param1 NONE
 * @return mcuy value.
 */
uint32_t bk_jpegdec_get_mcuy(void);

/**
 * @brief  bk_jpegdec_set_mcuy
 * @param1  mcuy value
 * @return  none 
 */
void bk_jpegdec_set_mcuy(uint32_t value);

/**
 * @brief  bk_jpegdec_set_mcux
 * @param1  mcux value
 * @return  none .
 */
void bk_jpegdec_set_mcux(uint32_t value);

/**
 * @brief  bk_jpegdec_set_dcuv
 * @param1  deuv value
 * @return  none .
 */
void bk_jpegdec_set_dcuv(uint32_t value);


/**
 * @brief  bk_jpegdec_close
 * @param1  deuv value
 * @return  none .
 */
void bk_jpegdec_close(void);


/**
 * @brief  bk_jpegdec_start
 * @param1  deuv value
 * @return  none .
 */
void bk_jpegdec_start(void);

/**
 * @brief  bk_jpeg_dec_isr_unregister
 * @param1  none
 * @return  none .
 */
void bk_jpeg_dec_isr_unregister(void);

/**
 * @brief  bk_jpeg_dec_isr_register
 * @param1  jpeg_dec_isr
 * @return  none .
 */
void bk_jpeg_dec_isr_register(int_isr_t jpeg_dec_isr);


#ifdef __cplusplus
}
#endif









