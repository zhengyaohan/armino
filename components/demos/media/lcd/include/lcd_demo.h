#ifndef _LCD_DEMO_H_
#define _LCD_DEMO_H_

#define JPEG_DMA_CH DMA_ID_0
#define LCD_DMA_CH  DMA_ID_1

typedef struct {
	volatile uint32_t dma_int_cnt;
	uint32_t dma_transfer_cnt;
	volatile uint32_t lcd_isr_cnt;
	uint32_t dma_transfer_len;
	volatile uint32_t dma_frame_end_flag;
} dma_transfer_t;

#endif

