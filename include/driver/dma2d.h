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
#include <components/log.h>

#include <driver/hal/hal_dma2d_types.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @defgroup DMA2D API
  * @{
  */


/**
  * @brief  initializes the DMA2D peripheral registers
  * @param  hdma2d pointer to a DMA2D_HandleTypeDef structure that contains
  *                 the configuration information for the DMA2D.
  * @retval None
  */
bk_err_t bk_dma2d_driver_init(DMA2D_HandleTypeDef *dma2d);

/**
  * @brief  Deinitializes the DMA2D peripheral registers to their default reset values.
  * @retval None
  */
bk_err_t bk_dma2d_driver_deinit(void);

/**
  * @brief  Configure the DMA2D Layer according to the specified
  * @param  dma2d Pointer to a DMA2D_HandleTypeDef structure that contains the configuration information for the DMA2D.
  * @param  LayerIdx DMA2D Layer index.
  *                   This parameter can be one of the following values:
  *                   DMA2D_BACKGROUND_LAYER(0) / DMA2D_FOREGROUND_LAYER(1)
  * @retval bk_err_t status
  */
bk_err_t bk_dma2d_driver_layer_config(DMA2D_HandleTypeDef *dma2d, uint32_t LayerIdx);

/**
  * @brief  Start the DMA2D Transfer
  * @param  dma2d     Pointer to a DMA2D_HandleTypeDef structure that contains  the configuration information for the DMA2D.
  * @param  pdata      Configure the source memory Buffer address if
  *                     the Memory-to-Memory or Memory-to-Memory with pixel format
  *                     conversion mode is selected, or configure
  *                     the color value if Register-to-Memory mode is selected.
  * @param  dst_addr The destination memory Buffer address.
  * @param  Width      The width of data to be transferred from source to destination (expressed in number of pixels per line).
  * @param  Height     The height of data to be transferred from source to destination (expressed in number of lines).
  * @retval bk_err_t status
  */
bk_err_t bk_dma2d_driver_start_transfer(DMA2D_HandleTypeDef *dma2d, uint32_t pdata, uint32_t dst_addr, uint32_t width, uint32_t height);

/**
  * @brief  Start the multi-source DMA2D Transfer.
  * @param  dma2d      Pointer to a DMA2D_HandleTypeDef structure that contains  the configuration information for the DMA2D.
  * @param  src_addr1 The source memory Buffer address for the foreground layer.
  * @param  src_addr2 The source memory Buffer address for the background layer.
  * @param  dst_addr  The destination memory Buffer address.
  * @param  Width       The width of data to be transferred from source to destination (expressed in number of pixels per line).
  * @param  Height      The height of data to be transferred from source to destination (expressed in number of lines).
  * @retval bk_err_t status
  */
bk_err_t bk_dma2d_driver_blending_start(DMA2D_HandleTypeDef *dma2d, uint32_t src_addr1, uint32_t src_addr2, uint32_t dst_addr, uint32_t width,  uint32_t height);

/**
  * @brief  bk_dma2d_wait_transfer_done
  * @retval return 0: transfer done;  return others not transfer done 
  */
bool bk_dma2d_wait_transfer_done(void);

/**
  * @brief  bk_dma2d_int_config.
  * @param  int_type  select from DMA2D_INT_TYPE, include int type:
                     DMA2D_CFG_ERROR
                     DMA2D_CLUT_TRANS_COMPLETE
                     DMA2D_CLUT_TRANS_ERROR
                     DMA2D_WARTERMARK_INT
                     DMA2D_TRANS_COMPLETE
                     DMA2D_TRANS_ERROR
  * @param  enable int
  * @retval bk_err_t status
  */
bk_err_t  bk_dma2d_int_config(DMA2D_INT_TYPE int_type, bool enable);

/**
  * @brief  bk_dma2d_int_status_get.
  * @retval return <value> is all int status, can used by <value> & DMA2D_INT_STATUS check which int triggered
                      typedef enum
                      {
                          DMA2D_TRANS_ERROR_STATUS = 0x1,
                          DMA2D_TRANS_COMPLETE_STATUS,
                          DMA2D_WARTERMARK_INT_STATUS,
                          DMA2D_CLUT_TRANS_ERROR_STATUS,
                          DMA2D_CLUT_TRANS_COMPLETE_STATUS,
                          DMA2D_CFG_ERROR_STATUS
                      }DMA2D_INT_STATUS;
  */
bk_err_t bk_dma2d_int_status_get(void);

/**
  * @brief  clear int status
  * @param  int_status select from DMA2D_INT_STATUS include:
                          DMA2D_TRANS_ERROR_STATUS
                          DMA2D_TRANS_COMPLETE_STATUS
                          DMA2D_WARTERMARK_INT_STATUS
                          DMA2D_CLUT_TRANS_ERROR_STATUS
                          DMA2D_CLUT_TRANS_COMPLETE_STATUS
                          DMA2D_CFG_ERROR_STATUS

  * @retval bk_err_t status
  */
bk_err_t  bk_dma2d_int_status_clear(DMA2D_INT_STATUS int_status);

/**
  * @brief bk_dma2d_suspend
  * @param suspend
  * @retval bk_err_t status
  */
bk_err_t bk_driver_dma2d_suspend(bool suspend);

/**
  * @brief bk_dma2d_abort
  * @param abort
  * @retval bk_err_t status
  */
bk_err_t bk_driver_dma2d_abort(bool abort);

/**
  * @brief  register dma2d cpu int isr
  * @param  dma2d_isr the function you registr isr
  * @retval bk_err_t status
  */
bk_err_t bk_dma2d_isr_register(dma2d_isr_t dma2d_isr);

#if (USE_HAL_DMA2D_REGISTER_CALLBACKS == 1)
/**
  * @brief  register dma2d int type isr ,user option function
            open the macro  #define USE_HAL_DMA2D_REGISTER_CALLBACKS 1
  * @param  isr_id based on int type, a int type can register a isr, select from:
                             typedef enum
                             {
                                  DMA2D_CFG_ERROR_ISR = 0,
                                  DMA2D_CLUT_TRANS_COMPLETE_ISR,
                                  DMA2D_CLUT_TRANS_ERROR_ISR,
                                  DMA2D_WARTERMARK_INT_ISR,
                                  DMA2D_TRANS_COMPLETE_ISR,
                                  DMA2D_TRANS_ERROR_ISR,
                             }DMA2D_ISR_ID;
  * @param  cb_isr the user register int callback function
  * @retval bk_err_t status
  */
bk_err_t bk_dma2d_register_int_callback_isr(DMA2D_ISR_ID isr_id, dma2d_isr_t cb_isr);
#endif

/**
  * @brief dma2d reg to mem mode,dma2d fill function
           attention: data format is rgb565
  * @param1  the display framebuffer base addr, by defult psram 0x60000000
  * @param2  the lcd display x , x <= 320
  * @param3  the lcd display y , y <= 480
  * @param4  the display width , width <= 320 - x
  * @param5  the display high , high <= 480 - y
  * @param6  color the fill color
  * @return none
  */
void dma2d_fill(uint32_t frameaddr, uint16_t x, uint16_t y, uint16_t width, uint16_t high, uint16_t color);

/**
  * @brief dma2d mem to mem mode, dma2d memcpy function
           attention: data format is rgb565
  * @param1  input pixelformat, copy data format
  * @param2  Psrc, source data addr
  * @param3  Pdst, destination data addr
  * @param4  xsize the display mem x size
  * @param5  ysize the display mem y size
  * @param6  src_offline. copy src mem data from offsetline addr,offsetline is calculate by pixel
  * @param6  dest_offline. src mem data copy to destination offsetline addr, offsetline is calculate by pixel
  * @return none
  */
void dma2d_memcpy(uint32_t pixelformat, void *Psrc, void *Pdst, uint32_t xsize, uint32_t ysize, uint32_t src_offline, uint32_t dest_offline);

/**
  * @brief dma2d mem to mem with blending mode
           attention: data format is rgb565
  * @param1  pFgaddr, foreground layer data addr
  * @param2  pBgaddr, background layer data addr
  * @param3  Pdst, blend destination data addr
  * @param4  fg_offline foreground layer blend from fg_offline addr, offsetline is calculate by pixel
  * @param5  bg_offline bg_offline layer blend from fg_offline addr,offsetline is calculate by pixel
  * @param6  dest_offline. blend complete,and display on dest addr edst_offline
  * @param7  xsize the display lcd x size
  * @param8  ysize the display lcd y size
  * @param9  alpha_value  foreground layer alpha_value
  * @return none
  */
void dma2d_mix_colors(void *pFgaddr, void *pBgaddr, void *pDst,
							uint32_t fg_offline, uint32_t bg_offline, uint32_t dest_offline,
							uint16_t xsize, uint16_t ysize, int8_t alpha_value);

/**
  * @brief  DMA2D API END
  */

#ifdef __cplusplus 
}
#endif


