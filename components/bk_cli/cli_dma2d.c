#include <os/os.h>

#include "cli.h"
#include <driver/int.h>
#include <common/bk_err.h>
#include "sdkconfig.h"
#include <driver/dma2d.h>
#include "cli_dma2d.h"
#include <driver/psram.h>
#include <driver/dma.h>
#include "bk_general_dma.h"
#include "driver/lcd_disp_types.h"


static volatile uint32_t transferErrorDetected = 0;    /**< Set to 1 if an error transfer is detected */
static volatile uint32_t transferCompleteDetected = 0; /**< Set to 1 if the DMA2D transfer complete is detected */
static uint32_t   offset_address_area_blended_image_in_lcd_buffer =  0;
static volatile uint32_t     dma_int_cnt = 0;
static volatile uint32_t     dma_int_flag = 0;
extern const uint16_t rgb_565_green[640];
extern volatile uint32_t *sm1_addr;
extern volatile uint32_t *sm2_addr;
static volatile uint32_t PSRAM_BASEADDR =   0x60000000;

#if (USE_HAL_DMA2D_REGISTER_CALLBACKS == 1)
static void mda2d_r2m_transfer_error(DMA2D_HandleTypeDef *dma2d);
static void mda2d_r2m_transfer_complete(DMA2D_HandleTypeDef *dma2d);
#endif

static void dma_fill_finish_isr(dma_id_t id)
{
	//addAON_GPIO_Reg0x3 = 0x2;
	dma_int_cnt++;
	if(dma_int_cnt == 5)
	{
		dma_int_cnt = 0;
		dma_int_flag = 1;
		addGENER_DMA_Reg0x2 =(uint32)PSRAM_BASEADDR;
		//addGENER_DMA_Reg0x2 =(uint32)&RGB565_320x480[0];
		//bk_dma_start(0);
	}
	else {
		
		//addGENER_DMA_Reg0x2 = (uint32)&RGB565_320x480[0] + (uint32_t)(61440 * dma_int_cnt);
		addGENER_DMA_Reg0x2 = (uint32)PSRAM_BASEADDR + (uint32_t)(61440 * dma_int_cnt);
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(0));
		bk_dma_start(0);
	}
	//addAON_GPIO_Reg0x3 = 0x0;
}

void dma2d_isr()
{

	uint32_t int_status;
	int_status = bk_dma2d_int_status_get();
	//os_printf("int_status = %x \r\n", int_status);
	//addAON_GPIO_Reg0x3 = 0x2;

	if (int_status & DMA2D_CFG_ERROR) {
		bk_dma2d_int_status_clear(DMA2D_CFG_ERROR_STATUS);
		bk_dma2d_int_config(DMA2D_CFG_ERROR, 0);
		os_printf("transferErrorDetected \r\n");
	}

	/**< Transfer Error Interrupt management ***************************************/
	if (int_status & DMA2D_TRANS_ERROR_STATUS) {
		transferErrorDetected = 1;
		bk_dma2d_int_status_clear(DMA2D_TRANS_ERROR_STATUS);
		bk_dma2d_int_config(DMA2D_TRANS_ERROR, 0);
		os_printf("transferErrorDetected \r\n");
	}

	if (int_status & DMA2D_TRANS_COMPLETE_STATUS) {
		transferCompleteDetected = 1;
		bk_dma2d_int_status_clear(DMA2D_TRANS_COMPLETE_STATUS);
		bk_dma2d_int_config(DMA2D_TRANS_COMPLETE, 0);
		//os_printf("transferCompleteDetected \r\n");
	}
	//addAON_GPIO_Reg0x3 = 0x0;
}

/**
  * @brief  Compares two buffers.
  * @param  pBuffer1, pBuffer2: buffers to be compared.
  * @param  BufferLength: buffer's length.
  * @retval  0: pBuffer1 identical to pBuffer2
  *          1: pBuffer1 differs from pBuffer2
  */
bk_err_t Buffercmp(uint32_t* pBuffer1, uint32_t* pBuffer2, uint32_t BufferLength)
{
	while (BufferLength--) {
		if (*pBuffer1 != *pBuffer2)
		return BK_FAIL;
		pBuffer1++;
		pBuffer2++;
	}
	return BK_OK;
}


static void  dma_start_transfer(void)
{
	dma_config_t dma_config = {0};
	dma_int_cnt  = 0;
	dma_config.mode = DMA_WORK_MODE_SINGLE;
	dma_config.chan_prio = 0;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = DMA_DATA_WIDTH_32BITS;
	dma_config.src.start_addr = (uint32) PSRAM_BASEADDR;
	dma_config.src.end_addr = (uint32_t) PSRAM_BASEADDR + 61440;
	dma_config.dst.start_addr = (uint32_t) &REG_DISP_DAT_FIFO;
	dma_config.dst.dev = DMA_DEV_LCD_DATA;
	dma_config.dst.width = DMA_DATA_WIDTH_16BITS;
	
	BK_LOG_ON_ERR(bk_dma_init(0, &dma_config));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(0, 61440));
	BK_LOG_ON_ERR(bk_dma_register_isr(0, NULL, dma_fill_finish_isr));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(0));
	BK_LOG_ON_ERR(bk_dma_start(0));
}

static void lcd_fill(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint16_t x, y, w, h, color;

	uint32_t frameaddr = 0x60000000;
	x = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	CLI_LOGI("x = %d \r\n", x);

	y = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	CLI_LOGI("y = %d \r\n", y);

	w = os_strtoul(argv[3], NULL, 10) & 0xFFFF;
	CLI_LOGI("w = %d \r\n", w);

	
	h= os_strtoul(argv[4], NULL, 10) & 0xFFFF;
	CLI_LOGI("h = %d \r\n", h);
	
	color = os_strtoul(argv[5], NULL, 16) & 0xFFFF;
	CLI_LOGI("fill_color = %x \r\n", color);
	
	dma2d_fill(frameaddr, x, y, w, h, color);
	dma_start_transfer();
}


#if (USE_HAL_DMA2D_REGISTER_CALLBACKS == 1)
static void mda2d_r2m_transfer_error(DMA2D_HandleTypeDef *dma2d)
{
	bk_dma2d_int_status_clear(DMA2D_TRANS_ERROR_STATUS);
	bk_dma2d_int_config(DMA2D_TRANS_ERROR, DISABLE);
}
static void mda2d_r2m_transfer_complete(DMA2D_HandleTypeDef *dma2d)
{
	bk_dma2d_int_status_clear(DMA2D_TRANS_COMPLETE_STATUS);
	bk_dma2d_int_config(DMA2D_TRANS_COMPLETE, DISABLE);
}
#endif



static void dma2d_display(uint16_t index, void *pdst)
{
	void * pdist = (void *) pdst;
	uint16_t *pstart = (uint16_t *)love;
	pstart += (index/PICTURE_COUNT_ROW) * (PICTURE_WIDTH_PIXEL * PICTURE_HIGHT * PICTURE_COUNT_ROW);
	pstart += (index % PICTURE_COUNT_ROW) * PICTURE_WIDTH_PIXEL;
	uint32_t offlinesrc = (PICTURE_COUNT_ROW - 1) * PICTURE_WIDTH_PIXEL;
	uint32_t offlinedist = 320 - PICTURE_WIDTH_PIXEL;

	dma2d_memcpy(DMA2D_OUTPUT_RGB565, (void *)pstart, pdist, PICTURE_WIDTH_PIXEL, PICTURE_HIGHT, offlinesrc, offlinedist);
}

static void dma2d_memcpy_display(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t pdst;
	
	pdst = os_strtoul(argv[1], NULL, 16) & 0xFFFFFFFF;
	os_printf("pdst = 0x%x . \r\n", pdst);
		
	//while(1) {
	for (int i = 0; i<  PICTURE_COUNT ; i++) {
			dma2d_display(i, (void *)pdst);
		//cpu_delay(50);
		//8080 need use dma move
			dma_start_transfer();
		}
	//}
}

static void bk_example_dma2d_mem_to_mem(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("test start: dma2d_mem_to_mem \r\n");
	DMA2D_HandleTypeDef Dma2dHandle = {0};

	/*##-1- Configure the DMA2D Mode, Output Color Mode and output offset #############*/
	Dma2dHandle.init.Mode         = DMA2D_M2M;             /**< Mode Memory To Memory */
	Dma2dHandle.init.ColorMode    = DMA2D_OUTPUT_ARGB4444; /**< Output color mode is ARGB4444 : 16 bpp */
	Dma2dHandle.init.OutputOffset = 0x0;                   /**< No offset on output */
	Dma2dHandle.init.RedBlueSwap   = DMA2D_RB_REGULAR;     /**< No R&B swap for the output image */
	Dma2dHandle.init.AlphaInverted = DMA2D_REGULAR_ALPHA;  /**< No alpha inversion for the output image */

	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaMode = DMA2D_NO_MODIF_ALPHA;      /**< Keep original Alpha from ARGB4444 input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputAlpha = 0xFF;                     /**< Fully opaque */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputColorMode = DMA2D_INPUT_ARGB4444; /**< Input color is ARGB4444 : 16 bpp */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputOffset = 0x0;                     /**< No offset in input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].RedBlueSwap   = DMA2D_RB_REGULAR;      /**< No R&B swap for the input image */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaInverted = DMA2D_REGULAR_ALPHA;   /**< No alpha inversion for the input image */

	bk_dma2d_driver_init(&Dma2dHandle);
	bk_dma2d_driver_layer_config(&Dma2dHandle, DMA2D_FOREGROUND_LAYER);

	bk_dma2d_int_config(DMA2D_CFG_ERROR | DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE ,1);
	bk_dma2d_isr_register(dma2d_isr);

	bk_dma2d_driver_start_transfer(&Dma2dHandle,
                        (uint32_t)&MemToMemInputBuff,  /**< Source memory buffer	   */ 
                        (uint32_t)&MemToMemOutputBuff, /**< Destination memory buffer */
                        M2M_LAYER_SIZE_X,              /**< width of buffer in pixels */
                        M2M_LAYER_SIZE_Y);             /**< height of buffer in lines */

	while (bk_dma2d_wait_transfer_done()) {
	}

	while ((transferCompleteDetected == 0) && (transferErrorDetected == 0)) {
	/**< wait until DMA transfer complete or transfer error */
	}
	if (transferErrorDetected == 1) {
	  /*transfer error detected */
	CLI_LOGI("mem to mem test is error\r\n");
	}
	else {
		if (Buffercmp(MemToMemInputBuff, MemToMemOutputBuff, ((M2M_LAYER_SIZE_X * M2M_LAYER_SIZE_Y * TWO_BYTE_PER_PIXEL) / 4)) != 0) {
			/**< Source and destination data not matching  */
			CLI_LOGI("mem to mem test is error\r\n");
		}
		else {
			/**< Source and destination data are matching  */
			CLI_LOGI("mem to mem test is success\r\n");
		}
	}
}

static void bk_example_dma2d_rgb565_to_argb8888pixel(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	//uint32_t psram_mode = 0;
	uint32_t RedBlueSwapConfig = 0;


	RedBlueSwapConfig = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	CLI_LOGI("RedBlueSwapConfig = %d \r\n", RedBlueSwapConfig);
	//memset((void *)sm1_addr, 0, 640);

	//psram_mode = 0x00054043;
	//bk_psram_init(psram_mode);

	DMA2D_HandleTypeDef Dma2dHandle = {0};

	/*##-1- Configure the DMA2D Mode, Output Color Mode and output offset #############*/
	Dma2dHandle.init.Mode         = DMA2D_M2M_PFC;                  /**< DMA2D Mode memory to memory  with PFC*/
	Dma2dHandle.init.ColorMode    = DMA2D_OUTPUT_ARGB8888;          /**< output format of DMA2D */

	/**< Configure the output OutputOffset to (LCD width - image width) */
	/**< Output offset in pixels == nb of pixels to be added at end of line to come to the  */
	/**< first pixel of the next line : on the output side of the DMA2D computation  */
	Dma2dHandle.init.OutputOffset = 0; 
	Dma2dHandle.init.RedBlueSwap   = DMA2D_RB_REGULAR;               /**< No R&B swap for the output image */
	Dma2dHandle.init.AlphaInverted = DMA2D_REGULAR_ALPHA;            /**< No alpha inversion for the output image */

	/**< Foreground layer Configuration */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaMode = DMA2D_NO_MODIF_ALPHA;    /**< Keep original Alpha from ARGB4444 input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputAlpha = 0xFF;                   /**< Fully opaque */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputColorMode = DMA2D_INPUT_RGB565; /**< Input color is RGB565 : 16 bpp */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputOffset = 0x0;                   /**< No offset in input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].RedBlueSwap   = RedBlueSwapConfig;   /**< No R&B swap for the input image */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaInverted = DMA2D_REGULAR_ALPHA; /**< No alpha inversion for the input image */

	bk_dma2d_driver_init(&Dma2dHandle);
	bk_dma2d_driver_layer_config(&Dma2dHandle, DMA2D_FOREGROUND_LAYER);

	//bk_dma2d_int_config(DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE ,1);
	//bk_dma2d_isr_register(dma2d_isr);

	bk_dma2d_driver_start_transfer(&Dma2dHandle,
                                  (uint32_t)PSRAM_BASEADDR, /**< Source buffer in format RGB565 and size 320x240      */
                                  (uint32_t)0X60050000,/**< framebuf+2*(LCD_X_SIZE*sy+sx)*/
                                  IMAGE_SIZE_X,           /**< width in pixels  */
                                  IMAGE_SIZE_Y);        /**< height in pixels */

	while (bk_dma2d_wait_transfer_done()) {}
}

static void bk_example_dma2d_argb8888_to_rgb565pixel(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	//uint32_t psram_mode = 0;
	uint32_t RedBlueSwapConfig = 0;

	RedBlueSwapConfig = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	CLI_LOGI("RedBlueSwapConfig = %d \r\n", RedBlueSwapConfig);
	//memset((void *)sm2_addr, 0, 640);

	//psram_mode = 0x00054043;
	//bk_psram_init(psram_mode);

	DMA2D_HandleTypeDef Dma2dHandle = {0};

	/*##-1- Configure the DMA2D Mode, Output Color Mode and output offset #############*/
	Dma2dHandle.init.Mode         = DMA2D_M2M_PFC;                  /**< DMA2D Mode memory to memory  with PFC*/
	Dma2dHandle.init.ColorMode    = DMA2D_OUTPUT_RGB565;          /**< output format of DMA2D */

	/**< Configure the output OutputOffset to (LCD width - image width) */
	/**< Output offset in pixels == nb of pixels to be added at end of line to come to the  */
	/**< first pixel of the next line : on the output side of the DMA2D computation  */
	Dma2dHandle.init.OutputOffset = 0; 
	Dma2dHandle.init.RedBlueSwap   = DMA2D_RB_REGULAR;               /**< No R&B swap for the output image */
	Dma2dHandle.init.AlphaInverted = DMA2D_REGULAR_ALPHA;            /**< No alpha inversion for the output image */

	/**< Foreground layer Configuration */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaMode = DMA2D_NO_MODIF_ALPHA;    /**< Keep original Alpha from ARGB4444 input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputAlpha = 0xFF;                   /**< Fully opaque */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputColorMode = DMA2D_INPUT_ARGB8888; /**< Input color is RGB565 : 16 bpp */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputOffset = 0x0;                   /**< No offset in input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].RedBlueSwap   = RedBlueSwapConfig;   /**< No R&B swap for the input image */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaInverted = DMA2D_REGULAR_ALPHA; /**< No alpha inversion for the input image */

	bk_dma2d_driver_init(&Dma2dHandle);
	bk_dma2d_driver_layer_config(&Dma2dHandle, DMA2D_FOREGROUND_LAYER);

	//bk_dma2d_int_config(DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE ,1);
	//bk_dma2d_isr_register(dma2d_isr);

	bk_dma2d_driver_start_transfer(&Dma2dHandle,
                                  (uint32_t)0X60050000, /**< Source buffer in format RGB565 and size 320x240      */
                                  (uint32_t)PSRAM_BASEADDR,/**< framebuf+2*(LCD_X_SIZE*sy+sx)*/
                                  IMAGE_SIZE_X,           /**< width in pixels  */
                                  IMAGE_SIZE_Y);        /**< height in pixels */
	while (bk_dma2d_wait_transfer_done()) {}
	dma_start_transfer();
}

static void bk_example_dma2d_rgb565_to_rgb888pixel(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	//uint32_t psram_mode = 0;
	uint32_t RedBlueSwapConfig = 0;


	RedBlueSwapConfig = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	CLI_LOGI("RedBlueSwapConfig = %d \r\n", RedBlueSwapConfig);
	//memset((void *)sm1_addr, 0, 640);

	//psram_mode = 0x00054043;
	//bk_psram_init(psram_mode);

	DMA2D_HandleTypeDef Dma2dHandle = {0};

	/*##-1- Configure the DMA2D Mode, Output Color Mode and output offset #############*/
	Dma2dHandle.init.Mode         = DMA2D_M2M_PFC;                  /**< DMA2D Mode memory to memory  with PFC*/
	Dma2dHandle.init.ColorMode    = DMA2D_OUTPUT_RGB888;          /**< output format of DMA2D */

	/**< Configure the output OutputOffset to (LCD width - image width) */
	/**< Output offset in pixels == nb of pixels to be added at end of line to come to the  */
	/**< first pixel of the next line : on the output side of the DMA2D computation  */
	Dma2dHandle.init.OutputOffset = 0; 
	Dma2dHandle.init.RedBlueSwap   = DMA2D_RB_REGULAR;               /**< No R&B swap for the output image */
	Dma2dHandle.init.AlphaInverted = DMA2D_REGULAR_ALPHA;            /**< No alpha inversion for the output image */

	/**< Foreground layer Configuration */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaMode = DMA2D_NO_MODIF_ALPHA;    /**< Keep original Alpha from ARGB4444 input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputAlpha = 0xFF;                   /**< Fully opaque */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputColorMode = DMA2D_INPUT_RGB565; /**< Input color is RGB565 : 16 bpp */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputOffset = 0x0;                   /**< No offset in input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].RedBlueSwap   = RedBlueSwapConfig;   /**< No R&B swap for the input image */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaInverted = DMA2D_REGULAR_ALPHA; /**< No alpha inversion for the input image */

	bk_dma2d_driver_init(&Dma2dHandle);
	bk_dma2d_driver_layer_config(&Dma2dHandle, DMA2D_FOREGROUND_LAYER);

	//bk_dma2d_int_config(DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE ,1);
	//bk_dma2d_isr_register(dma2d_isr);

	bk_dma2d_driver_start_transfer(&Dma2dHandle,
                                  (uint32_t)PSRAM_BASEADDR, /**< Source buffer in format RGB565 and size 320x240      */
                                  (uint32_t)0X60050000,/**< framebuf+2*(LCD_X_SIZE*sy+sx)*/
                                  IMAGE_SIZE_X,           /**< width in pixels  */
                                  IMAGE_SIZE_Y);        /**< height in pixels */

	while (bk_dma2d_wait_transfer_done()) {}
}

static void bk_example_dma2d_rgb888_to_rgb565pixel(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	//uint32_t psram_mode = 0;
	uint32_t RedBlueSwapConfig = 0;

	RedBlueSwapConfig = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	CLI_LOGI("RedBlueSwapConfig = %d \r\n", RedBlueSwapConfig);
	//memset((void *)sm2_addr, 0, 640);

	//psram_mode = 0x00054043;
	//bk_psram_init(psram_mode);

	DMA2D_HandleTypeDef Dma2dHandle = {0};

	/*##-1- Configure the DMA2D Mode, Output Color Mode and output offset #############*/
	Dma2dHandle.init.Mode         = DMA2D_M2M_PFC;                  /**< DMA2D Mode memory to memory  with PFC*/
	Dma2dHandle.init.ColorMode    = DMA2D_OUTPUT_RGB565;          /**< output format of DMA2D */

	/**< Configure the output OutputOffset to (LCD width - image width) */
	/**< Output offset in pixels == nb of pixels to be added at end of line to come to the  */
	/**< first pixel of the next line : on the output side of the DMA2D computation  */
	Dma2dHandle.init.OutputOffset = 0; 
	Dma2dHandle.init.RedBlueSwap   = DMA2D_RB_REGULAR;               /**< No R&B swap for the output image */
	Dma2dHandle.init.AlphaInverted = DMA2D_REGULAR_ALPHA;            /**< No alpha inversion for the output image */

	/**< Foreground layer Configuration */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaMode = DMA2D_NO_MODIF_ALPHA;    /**< Keep original Alpha from ARGB4444 input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputAlpha = 0xFF;                   /**< Fully opaque */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputColorMode = DMA2D_INPUT_RGB888; /**< Input color is RGB565 : 16 bpp */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].InputOffset = 0x0;                   /**< No offset in input */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].RedBlueSwap   = RedBlueSwapConfig;   /**< No R&B swap for the input image */
	Dma2dHandle.LayerCfg[DMA2D_FOREGROUND_LAYER].AlphaInverted = DMA2D_REGULAR_ALPHA; /**< No alpha inversion for the input image */

	bk_dma2d_driver_init(&Dma2dHandle);
	bk_dma2d_driver_layer_config(&Dma2dHandle, DMA2D_FOREGROUND_LAYER);

	//bk_dma2d_int_config(DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE ,1);
	//bk_dma2d_isr_register(dma2d_isr);

	bk_dma2d_driver_start_transfer(&Dma2dHandle,
                                  (uint32_t)0X60050000, /**< Source buffer in format RGB565 and size 320x240      */
                                  (uint32_t)PSRAM_BASEADDR,/**< framebuf+2*(LCD_X_SIZE*sy+sx)*/
                                  IMAGE_SIZE_X,           /**< width in pixels  */
                                  IMAGE_SIZE_Y);        /**< height in pixels */
	while (bk_dma2d_wait_transfer_done()) {}
	dma_start_transfer();
}


static void bk_example_alpha_blend(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	const uint8_t cnvFrames = 60;
	
	///calculate address of output
	offset_address_area_blended_image_in_lcd_buffer =  ((((LCD_Y_SIZE - DEMO_IMG_HEIGHT) / 2) * LCD_X_SIZE)
                                                    + ((LCD_X_SIZE - DEMO_IMG_WIDTH) / 2)) * 2;
	//uint16_t *pFb = &RGB565_320x480[0];
	//uint16_t *pDiSt = pFb + offset_address_area_blended_image_in_lcd_buffer;
	uint16_t *pDiSt = (void *)0x60000000 + offset_address_area_blended_image_in_lcd_buffer;

	uint16_t dist_offline = LCD_X_SIZE - DEMO_IMG_WIDTH;
	uint32_t nextimg = 1;
	uint32_t opa = 0;
	void * pFgaddr = 0;
	void * pBgaddr = 0;
	dma_int_flag = 0;

	os_printf("pFb addr = %x \r\n", offset_address_area_blended_image_in_lcd_buffer);
	os_printf("pDiSt addr = %x \r\n", pDiSt);

	while(1){
		if(nextimg){
				pFgaddr = (void*)yellow_color;
				pBgaddr = (void*)love;
		}
		else{
			pFgaddr = (void*)love;
			pBgaddr = (void*)yellow_color;
		}
				
		for(int i = 0; i < cnvFrames; i++) {
			opa = 255 * i / (cnvFrames-1);
			
			addAON_GPIO_Reg0x3 = 0x2;
			dma2d_mix_colors(pFgaddr, pBgaddr, pDiSt,0, 0, dist_offline,DEMO_IMG_WIDTH, DEMO_IMG_HEIGHT, 
							opa);
			addAON_GPIO_Reg0x3 = 0x0;
			dma_start_transfer();
			while(dma_int_flag == 0) {}
			dma_int_flag = 0;
		}
		nextimg = !nextimg;
	}
}

static void dma2d_fill_to_blend(void * pDiSt, uint16_t width, uint16_t high, uint16_t color)
{
	DMA2D_HandleTypeDef Dma2dHandle = {0};
	
	Dma2dHandle.init.Mode		   = DMA2D_R2M; 			 /**< Mode Register to Memory */
	Dma2dHandle.init.ColorMode	   = DMA2D_OUTPUT_RGB565;  /**< DMA2D Output color mode is ARGB4444 (16 bpp) */
	Dma2dHandle.init.OutputOffset  = 0;					 /**< No offset in output */
	Dma2dHandle.init.RedBlueSwap   = DMA2D_RB_REGULAR;		 /**< No R&B swap for the output image */
	Dma2dHandle.init.AlphaInverted = DMA2D_REGULAR_ALPHA;	 /**< No alpha inversion for the output image */
	bk_dma2d_driver_init(&Dma2dHandle);

	bk_dma2d_int_config(DMA2D_TRANS_ERROR | DMA2D_TRANS_COMPLETE ,1);
	bk_dma2d_isr_register(dma2d_isr);
	bk_dma2d_driver_start_transfer(&Dma2dHandle,
							color,						  /**< Color value is entered in ARGB8888 format: 0x00FFFF00 will yield a combination of Red and Green, that is yellow. */
							(uint32_t)pDiSt,	  /**< DMA2D output buffer */
							width, 					 /**< width of buffer in pixels */
							high); 
	while (transferCompleteDetected == 0) {;}
	transferCompleteDetected = 0;
}

static void bk_example_fill_alpha_blend(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	const uint8_t cnvFrames = 60;

	uint16_t image_x, image_y, image1_color, image2_color;
	
	image_x = os_strtoul(argv[1], NULL, 10) & 0xFFFF;
	image_y = os_strtoul(argv[2], NULL, 10) & 0xFFFF;
	image1_color = os_strtoul(argv[3], NULL, 16) & 0xFFFF;
	image2_color = os_strtoul(argv[4], NULL, 16) & 0xFFFF;

	uint16_t distx = (LCD_X_SIZE - image_x) / 2;
	uint16_t disty = (LCD_Y_SIZE - image_y)/ 2;
	uint16_t *pDiSt = (void *)PSRAM_BASEADDR + (distx + disty * LCD_X_SIZE) *2 ;

	dma2d_fill_to_blend((void *)0x60050000, image_x, image_y, image1_color);
	dma2d_fill_to_blend((void *)0x60100000, image_x, image_y, image2_color);

	uint16_t dist_offline = LCD_X_SIZE - image_x;
	uint32_t nextimg = 1;
	uint32_t opa = 0;
	void * pFgaddr = 0;
	void * pBgaddr = 0;
	dma_int_flag = 0;

	os_printf("pDiSt addr = %x \r\n", pDiSt);

	while(1){
		if(nextimg){
				pFgaddr = (void*)0x60050000;
				pBgaddr = (void*)0x60100000;
		}
		else{
			pFgaddr = (void*)0x60100000;
			pBgaddr = (void*)0x60050000;
		}
				
		for(int i = 0; i < cnvFrames; i++) {
			opa = 255 * i / (cnvFrames-1);
			addAON_GPIO_Reg0x3 = 0x2;
			dma2d_mix_colors(pFgaddr, pBgaddr, pDiSt, 0, 0, dist_offline,
			image_x, image_y, opa);
			/*
			os_printf("output addr = %x \r\n", addDMA2D_Reg0xf);
			os_printf("fg addr = %x \r\n", addDMA2D_Reg0x3);
			os_printf("bg addr = %x \r\n", addDMA2D_Reg0x5);
			
			os_printf("output offline = %x \r\n", addDMA2D_Reg0x10 & 0xffff);
			os_printf("fg offline = %x \r\n", addDMA2D_Reg0x4 & 0xffff);
			os_printf("bg offline = %x \r\n", addDMA2D_Reg0x6 & 0xffff);
			*/
			addAON_GPIO_Reg0x3 = 0x0;
			dma_start_transfer();
			while(dma_int_flag == 0) {}
			dma_int_flag = 0;
		}
		nextimg = !nextimg;
	}
}

static void bk_example_fill_2p_alpha_blend(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	const uint8_t cnvFrames = 60;
	uint32_t nextimg = 1;
	uint32_t opa = 0;
	void * pFgaddr = 0;
	void * pBgaddr = 0;
	dma_int_flag = 0;
	uint16_t *pDiSt = (void *)PSRAM_BASEADDR;

	dma2d_fill(0x60050000, 0, 0, 320, 480, 0xffff);
	dma2d_fill(0x60050000, 20, 20, 280, 440, 0xffe0);
	dma2d_fill(0x60050000, 60, 60, 200, 360, 0x001f);
	dma2d_fill(0x60050000, 100, 100, 120, 280, 0xf60F);
	dma2d_fill(0x60050000, 140, 140, 40, 200, 0x07ff);
	
	dma2d_fill(0x60100000, 0,0,320,480,0xffff);
	dma2d_fill(0x60100000, 270,80,10,340,0xf800);
	dma2d_fill(0x60100000, 220,420,60,10,0xf800);
	dma2d_fill(0x60100000, 220,80,10,350,0xf800);
	dma2d_fill(0x60100000, 220,250,60,10,0xf800);
	dma2d_fill(0x60100000, 120,80,60,10,0xf800);
	dma2d_fill(0x60100000, 180,80,10,350,0xf800);
	dma2d_fill(0x60100000, 120,420,60,10,0xf800);
	dma2d_fill(0x60100000, 30,80, 60,10,0xf800);
	dma2d_fill(0x60100000, 80,80, 10,350,0xf800);

	while(1){
		if(nextimg){
				pFgaddr = (void*)0x60050000;
				pBgaddr = (void*)0x60100000;
		}
		else{
			pFgaddr = (void*)0x60100000;
			pBgaddr = (void*)0x60050000;
		}
				
		for(int i = 0; i < cnvFrames; i++) {
			opa = 255 * i / (cnvFrames-1);
			addAON_GPIO_Reg0x3 = 0x2;
			dma2d_mix_colors(pFgaddr, pBgaddr, pDiSt, 0, 0, 0,
			320, 480, opa);
			addAON_GPIO_Reg0x3 = 0x0;
			dma_start_transfer();
			while(dma_int_flag == 0) {}
			dma_int_flag = 0;
		}
		nextimg = !nextimg;
	}
}

static void bk_example_fill_3p_alpha_blend(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	const uint8_t cnvFrames = 60;
	uint32_t nextimg = 1;
	uint32_t opa = 0;
	void * pFgaddr = 0;
	void * pBgaddr = 0;
	dma_int_flag = 0;
	uint16_t *pDiSt = (void *)PSRAM_BASEADDR;
	
	dma2d_fill(0x60050000, 0, 0, 320, 480, 0xffff);
	dma2d_fill(0x60050000, 20, 20, 280, 440, 0xffe0);
	dma2d_fill(0x60050000, 60, 60, 200, 360, 0x001f);
	dma2d_fill(0x60050000, 100, 100, 120, 280, 0xf60F);
	dma2d_fill(0x60050000, 140, 140, 40, 200, 0x07ff);


	
	dma2d_fill(0x60100000, 0,0,320,480,0xffff);
	dma2d_fill(0x60100000, 270,80,10,340,0xf800);
	dma2d_fill(0x60100000, 220,420,60,10,0xf800);
	dma2d_fill(0x60100000, 220,80,10,350,0xf800);
	dma2d_fill(0x60100000, 220,250,60,10,0xf800);
	dma2d_fill(0x60100000, 120,80,60,10,0xf800);
	dma2d_fill(0x60100000, 180,80,10,350,0xf800);
	dma2d_fill(0x60100000, 120,420,60,10,0xf800);
	dma2d_fill(0x60100000, 30,80, 60,10,0xf800);
	dma2d_fill(0x60100000, 80,80, 10,350,0xf800);


	dma2d_fill(0x60150000, 0,0,320,480,0x07e0);

	while(1){
		if(nextimg){
				pFgaddr = (void*)0x60100000;
				pBgaddr = (void*)0x60150000;
		}
		else{
			pFgaddr = (void*)0x60150000;
			pBgaddr = (void*)0x60050000;
		}
				
		for(int i = 0; i < cnvFrames; i++) {
			opa = 255 * i / (cnvFrames-1);
			addAON_GPIO_Reg0x3 = 0x2;
			dma2d_mix_colors(pFgaddr, pBgaddr, pDiSt, 0, 0, 0,
			320, 480, opa);
			addAON_GPIO_Reg0x3 = 0x0;
			dma_start_transfer();
			while(dma_int_flag == 0) {}
			dma_int_flag = 0;
		}
		nextimg = !nextimg;
	}
}



#define DMA2D_CNT (sizeof(s_dma2d_commands) / sizeof(struct cli_command))
static const struct cli_command s_dma2d_commands[] = {
	//{"lcd_multi_fill", "lcd_multi_fill", lcd_multi_fill},
	{"lcd_fill", "lcd_dma2d_blending", lcd_fill},

	{"dma2d_mem_to_mem", "dma2d_mem_to_mem \r\n", bk_example_dma2d_mem_to_mem},
	{"dma2d_memcpy_display", "dma2d_mem_to_mem \r\n", dma2d_memcpy_display},


	{"dma2d_565_to_8888", "dma2d_pfc = RedBlueSwapConfig", bk_example_dma2d_rgb565_to_argb8888pixel},
	{"dma2d_8888_to_565", "dma2d_pfc = RedBlueSwapConfig", bk_example_dma2d_argb8888_to_rgb565pixel},
	{"dma2d_565_to_888", "dma2d_pfc = RedBlueSwapConfig", bk_example_dma2d_rgb565_to_rgb888pixel},
	{"dma2d_888_to_565", "dma2d_pfc = RedBlueSwapConfig", bk_example_dma2d_rgb888_to_rgb565pixel},
		

	{"alpha_patrtial_blend", "bk_example_alpha_blend", bk_example_alpha_blend},
	{"alpha_blend2", "bk_example_alpha_blend", bk_example_fill_alpha_blend},
	
	//{"blending_picture", "blending_picture=P1,X,Y,W,H,Color", lcd_fill_for_blending},
	{"alpha_fill_blend_p2", "bk_example_alpha_blend", bk_example_fill_2p_alpha_blend},
	{"alpha_fill_blend_p3", "bk_example_alpha_blend", bk_example_fill_3p_alpha_blend},
};

int cli_dma2d_init(void)
{
	return cli_register_commands(s_dma2d_commands, DMA2D_CNT);
}





















