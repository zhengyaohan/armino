#ifndef _UART_H_
#define _UART_H_

#include "include.h"
#include "bk_fifo.h"
#include "bk_uart.h"

#define UART_DEBUG              0

#if UART_DEBUG
#define UART_PRT                 os_printf
#define UART_WPRT                warning_prf
#else
#define UART_PRT                 os_null_printf
#define UART_WPRT                os_null_printf
#endif

#define PRINTF_BUF_SIZE           (128)

#define UART_INDEX_1              (1)
#define UART_INDEX_2              (2)

#if (CONFIG_FPGA)
#define UART_SELECT_CFG            UART_INDEX_1
#else
#define UART_SELECT_CFG            UART_INDEX_2
#endif

#if (UART_SELECT_CFG == UART_INDEX_1)
#define IRQ_UART                   IRQ_UART1
#define PRI_IRQ_UART               PRI_IRQ_UART1
#define PWD_UART_CLK_BIT           PWD_UART1_CLK_BIT
#define GFUNC_MODE_UART            GFUNC_MODE_UART1
#define IRQ_UART_BIT               IRQ_UART1_BIT
#else  /* UART_INDEX_2 */
#define IRQ_UART                   IRQ_UART2
#define PRI_IRQ_UART               PRI_IRQ_UART2
#define PWD_UART_CLK_BIT           PWD_UART2_CLK_BIT
#define GFUNC_MODE_UART            GFUNC_MODE_UART2
#define IRQ_UART_BIT               IRQ_UART2_BIT
#endif

#define DEBUG_PRT_MAX_CNT          (16)

#define RX_RB_LENGTH               (128)
#define TX_RB_LENGTH               (64)

#define CARRIAGE_RETURN(buf, count) \
	do { \
		buf[count - 1] = '\r'; \
		buf[count] = '\n'; \
		buf[count + 1] = 0; \
		rc += 1; \
	} while (0)

// /* uart parameter config----start*/
// #define UART_BAUDRATE_3250000        3250000
// #define UART_BAUDRATE_2000000        2000000
// #define UART_BAUDRATE_921600         921600
// #define UART_BAUDRATE_460800         460800
// #define UART_BAUDRATE_230400         230400
// #define UART_BAUDRATE_115200         115200  //default
// #define UART_BAUDRATE_3000           3250
// #define UART_BAUDRATE_19200          19200

// #define UART_BAUD_RATE               UART_BAUDRATE_115200
#ifndef UART1_BAUD_RATE
#define UART1_BAUD_RATE              UART_BAUD_RATE
#endif
#ifndef UART2_BAUD_RATE
#define UART2_BAUD_RATE              UART_BAUD_RATE
#endif
#if (CONFIG_SOC_BK7271)
#ifndef UART3_BAUD_RATE
#define UART3_BAUD_RATE              UART_BAUD_RATE
#endif
#endif

#ifndef UART1_USE_FIFO_REC
#define UART1_USE_FIFO_REC           1
#endif
#ifndef UART2_USE_FIFO_REC
#define UART2_USE_FIFO_REC           1
#endif
#if (CONFIG_SOC_BK7271)
#ifndef UART3_USE_FIFO_REC
#define UART3_USE_FIFO_REC           1
#endif
#endif

#ifndef UART1_CLOCK_SELECT_DCO
#define UART1_CLOCK_SELECT_DCO       0
#endif
#ifndef UART2_CLOCK_SELECT_DCO
#define UART2_CLOCK_SELECT_DCO       0
#endif

#define UART_CLOCK_FREQ_10M          10000000
#define UART_CLOCK_FREQ_48M          48000000
#define UART_CLOCK_FREQ_24M          24000000
#define UART_CLOCK_FREQ_26M          26000000
#define UART_CLOCK_FREQ_52M          52000000
#define UART_CLOCK_FREQ_120M         120000000

#if (CONFIG_FPGA)
#define UART_CLOCK                   UART_CLOCK_FREQ_24M
#else
#define UART_CLOCK                   CONFIG_XTAL_FREQ
#endif

#define TX_FIFO_THRD                (0x40)
#define RX_FIFO_THRD                (0x10)

#define DEF_TX_EN                   0x1
#define DEF_RX_EN                   0x1
#define DEF_IRDA_MODE               0x0    // 0:uart mode  1:IRDA MODE
#define DEF_DATA_LEN                0x3    // 0=5bit, 1=6bit, 2=7bit, 3=8bit
#define DEF_PARITY_EN               0x0    // 0=no parity  1: enable parity
#define DEF_PARITY_MODE             0x0    // 0:odd  1: even
#define DEF_STOP_BIT                0x0    // 1bit

#define FLOW_CTRL_HIGH_CNT          (96)
#define FLOW_CTRL_LOW_CNT           (32)

#define DEBUG_TX_FIFO_MAX_COUNT     16

/* uart parameter config----end*/

typedef struct _uart_ {
	UINT16 status;

	kfifo_ptr_t rx;

	kfifo_ptr_t tx;
} UART_S, *UART_PTR;


#if (CONFIG_DEBUG_FIRMWARE)
#define DEAD_WHILE()  do{           \
		while(1);\
	}while(0)
#else
#define DEAD_WHILE()  do{           \
		os_printf("dead\r\n");\
	}while(0)
#endif

#if (UART_SELECT_CFG == UART_INDEX_1)
#define UART_BASE_ADDR                       (0x0802100)
#else
#define UART_BASE_ADDR                       (0x0802200)
#endif

#define UART1_BASE_ADDR                      (0x0802100)

#if (CONFIG_SOC_BK7271)
#define UART2_BASE_ADDR                      (0x0802140)
#define UART3_BASE_ADDR                      (0x0802180)
#else
#define UART2_BASE_ADDR                      (0x0802200)
#endif

#define REG_UART1_CONFIG                     (UART1_BASE_ADDR + 4 * 0)
#define REG_UART2_CONFIG                     (UART2_BASE_ADDR + 4 * 0)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_CONFIG                     (UART3_BASE_ADDR + 4 * 0)
#endif

#define UART_TX_ENABLE                         (1 << 0)
#define UART_RX_ENABLE                         (1 << 1)
#define UART_IRDA                              (1 << 2)
#define UART_DATA_LEN_POSI                     (3)
#define UART_DATA_LEN_MASK                     (0x03)
#define UART_PAR_EN                            (1 << 5)
#define UART_PAR_ODD_MODE                      (1 << 6)
#define UART_STOP_LEN_2                        (1 << 7)
#define UART_CLK_DIVID_POSI                    (8)
#define UART_CLK_DIVID_MASK                    (0x1FFF)

#define REG_UART1_FIFO_CONFIG                 (UART1_BASE_ADDR + 4 * 1)
#define REG_UART2_FIFO_CONFIG                 (UART2_BASE_ADDR + 4 * 1)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_FIFO_CONFIG                 (UART3_BASE_ADDR + 4 * 1)
#endif
#define TX_FIFO_THRESHOLD_MASK                 (0xFF)
#define TX_FIFO_THRESHOLD_POSI                 (0)
#define RX_FIFO_THRESHOLD_MASK                 (0xFF)
#define RX_FIFO_THRESHOLD_POSI                 (8)
#define RX_STOP_DETECT_TIME_MASK               (0x03)
#define RX_STOP_DETECT_TIME_POSI               (16)
#define RX_STOP_DETECT_TIME32                  (0)
#define RX_STOP_DETECT_TIME64                  (1)
#define RX_STOP_DETECT_TIME128                 (2)
#define RX_STOP_DETECT_TIME256                 (3)

#define REG_UART1_FIFO_STATUS                 (UART1_BASE_ADDR + 4 * 2)
#define REG_UART2_FIFO_STATUS                 (UART2_BASE_ADDR + 4 * 2)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_FIFO_STATUS                 (UART3_BASE_ADDR + 4 * 2)
#endif
#define TX_FIFO_COUNT_MASK                     (0xFF)
#define TX_FIFO_COUNT_POSI                     (0)
#define RX_FIFO_COUNT_MASK                     (0xFF)
#define RX_FIFO_COUNT_POSI                     (8)
#define TX_FIFO_FULL                           (1 << 16)
#define TX_FIFO_EMPTY                          (1 << 17)
#define RX_FIFO_FULL                           (1 << 18)
#define RX_FIFO_EMPTY                          (1 << 19)
#define FIFO_WR_READY                          (1 << 20)
#define FIFO_RD_READY                          (1 << 21)

#define REG_UART1_FIFO_PORT                   (UART1_BASE_ADDR + 4 * 3)
#define REG_UART2_FIFO_PORT                   (UART2_BASE_ADDR + 4 * 3)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_FIFO_PORT                   (UART3_BASE_ADDR + 4 * 3)
#endif
#define UART_TX_FIFO_DIN_MASK                  (0xFF)
#define UART_TX_FIFO_DIN_POSI                  (0)
#define UART_RX_FIFO_DOUT_MASK                 (0xFF)
#define UART_RX_FIFO_DOUT_POSI                 (8)

#define REG_UART1_INTR_ENABLE                 (UART1_BASE_ADDR + 4 * 4)
#define REG_UART2_INTR_ENABLE                 (UART2_BASE_ADDR + 4 * 4)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_INTR_ENABLE                 (UART3_BASE_ADDR + 4 * 4)
#endif
#define TX_FIFO_NEED_WRITE_EN                  (1 << 0)
#define RX_FIFO_NEED_READ_EN                   (1 << 1)
#define RX_FIFO_OVER_FLOW_EN                   (1 << 2)
#define UART_RX_PARITY_ERR_EN                  (1 << 3)
#define UART_RX_STOP_ERR_EN                    (1 << 4)
#define UART_TX_STOP_END_EN                    (1 << 5)
#define UART_RX_STOP_END_EN                    (1 << 6)
#define UART_RXD_WAKEUP_EN                     (1 << 7)

#define REG_UART1_INTR_STATUS                 (UART1_BASE_ADDR + 4 * 5)
#define REG_UART2_INTR_STATUS                 (UART2_BASE_ADDR + 4 * 5)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_INTR_STATUS                 (UART3_BASE_ADDR + 4 * 5)
#endif
#define TX_FIFO_NEED_WRITE_STA                  (1 << 0)
#define RX_FIFO_NEED_READ_STA                   (1 << 1)
#define RX_FIFO_OVER_FLOW_STA                   (1 << 2)
#define UART_RX_PARITY_ERR_STA                  (1 << 3)
#define UART_RX_STOP_ERR_STA                    (1 << 4)
#define UART_TX_STOP_END_STA                    (1 << 5)
#define UART_RX_STOP_END_STA                    (1 << 6)
#define UART_RXD_WAKEUP_STA                     (1 << 7)

#define REG_UART1_FLOW_CONFIG                 (UART1_BASE_ADDR + 4 * 6)
#define REG_UART2_FLOW_CONFIG                 (UART2_BASE_ADDR + 4 * 6)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_FLOW_CONFIG                 (UART3_BASE_ADDR + 4 * 6)
#endif
#define FLOW_CTRL_LOW_CNT_MASK                   (0xFF)
#define FLOW_CTRL_LOW_CNT_POSI                   (0)
#define FLOW_CTRL_HIGH_CNT_MASK                  (0xFF)
#define FLOW_CTRL_HIGH_CNT_POSI                  (8)
#define FLOW_CONTROL_EN                          (1 << 16)
//only valid for uart1
#define RTS_POLARITY_SEL                         (1<<17)
#define CTS_POLARITY_SEL                         (1<<18)

#define REG_UART1_WAKE_CONFIG                 (UART1_BASE_ADDR + 4 * 7)
#define REG_UART2_WAKE_CONFIG                 (UART2_BASE_ADDR + 4 * 7)
#if (CONFIG_SOC_BK7271)
#define REG_UART3_WAKE_CONFIG                 (UART3_BASE_ADDR + 4 * 7)
#endif
#define UART_WAKE_COUNT_MASK                   (0x3FF)
#define UART_WAKE_COUNT_POSI                   (0)
#define UART_TXD_WAIT_CNT_MASK                 (0x3FF)
#define UART_TXD_WAIT_CNT_POSI                 (10)
#define UART_RXD_WAKE_EN                       (1 << 20)
#define UART_TXD_WAKE_EN                       (1 << 21)
#define RXD_NEGEDGE_WAKE_EN                    (1 << 22)

#define UART1_TX_WRITE_READY             (REG_READ(REG_UART1_FIFO_STATUS) & FIFO_WR_READY)
#define UART2_TX_WRITE_READY             (REG_READ(REG_UART2_FIFO_STATUS) & FIFO_WR_READY)
#if (CONFIG_SOC_BK7271)
#define UART3_TX_WRITE_READY             (REG_READ(REG_UART3_FIFO_STATUS) & FIFO_WR_READY)
#endif
#define UART1_TX_FIFO_COUNT             (((REG_READ(REG_UART1_FIFO_STATUS)) >> TX_FIFO_COUNT_POSI) & TX_FIFO_COUNT_MASK)
#define UART2_TX_FIFO_COUNT             (((REG_READ(REG_UART2_FIFO_STATUS)) >> TX_FIFO_COUNT_POSI) & TX_FIFO_COUNT_MASK)
#if (CONFIG_SOC_BK7271)
#define UART3_TX_FIFO_COUNT             (((REG_READ(REG_UART3_FIFO_STATUS)) >> TX_FIFO_COUNT_POSI) & TX_FIFO_COUNT_MASK)
#endif

#if (CONFIG_SOC_BK7271)
#define UART_WRITE_BYTE(ch,v)           do                                         \
	{                                          \
		v = (v & UART_TX_FIFO_DIN_MASK)        \
			<< UART_TX_FIFO_DIN_POSI;      \
		if (0 == ch)                           \
			REG_WRITE(REG_UART1_FIFO_PORT, v);\
		else if (1 == ch)                      \
			REG_WRITE(REG_UART2_FIFO_PORT, v);\
		else                                   \
			REG_WRITE(REG_UART3_FIFO_PORT, v);\
	}while(0)
#define UART_READ_BYTE(ch,v)            do                                         \
	{                                          \
		if (0 == ch)                           \
			v = (REG_READ(REG_UART1_FIFO_PORT)\
				 >> UART_RX_FIFO_DOUT_POSI) \
				& UART_RX_FIFO_DOUT_MASK;  \
		else if (1 == ch)                      \
			v = (REG_READ(REG_UART2_FIFO_PORT)\
				 >> UART_RX_FIFO_DOUT_POSI) \
				& UART_RX_FIFO_DOUT_MASK;  \
		else                                   \
			v = (REG_READ(REG_UART3_FIFO_PORT)\
				 >> UART_RX_FIFO_DOUT_POSI) \
				& UART_RX_FIFO_DOUT_MASK;  \
	}while(0)
#define UART_READ_BYTE_DISCARD(ch)      do                                         \
	{                                          \
		if(0 == ch)                            \
			REG_READ(REG_UART1_FIFO_PORT);     \
		else if (1 == ch)                      \
			REG_READ(REG_UART2_FIFO_PORT);     \
		else                                   \
			REG_READ(REG_UART3_FIFO_PORT);     \
	}while(0)
#else
#define UART_WRITE_BYTE(ch,v)           do                                         \
	{                                          \
		v = (v & UART_TX_FIFO_DIN_MASK)        \
			<< UART_TX_FIFO_DIN_POSI;      \
		if(0 == ch)                            \
			REG_WRITE(REG_UART1_FIFO_PORT, v);\
		else                                   \
			REG_WRITE(REG_UART2_FIFO_PORT, v);\
	}while(0)
#define UART_READ_BYTE(ch,v)            do                                         \
	{                                          \
		if(0 == ch)                            \
			v = (REG_READ(REG_UART1_FIFO_PORT)\
				 >> UART_RX_FIFO_DOUT_POSI)     \
				& UART_RX_FIFO_DOUT_MASK;      \
		else                                   \
			v = (REG_READ(REG_UART2_FIFO_PORT)\
				 >> UART_RX_FIFO_DOUT_POSI)     \
				& UART_RX_FIFO_DOUT_MASK;      \
	}while(0)
#define UART_READ_BYTE_DISCARD(ch)      do                                         \
	{                                          \
		if(0 == ch)                            \
			REG_READ(REG_UART1_FIFO_PORT);     \
		else                                   \
			REG_READ(REG_UART2_FIFO_PORT);     \
	}while(0)
#endif

#if (CONFIG_SOC_BK7271)
#define UART3_TX_FIFO_EMPTY_GET()       ((REG_READ(REG_UART3_FIFO_STATUS) & TX_FIFO_EMPTY)>>17)
#endif
#define UART2_TX_FIFO_EMPTY_GET()       ((REG_READ(REG_UART2_FIFO_STATUS) & TX_FIFO_EMPTY)>>17)
#define UART1_TX_FIFO_EMPTY_GET()       ((REG_READ(REG_UART1_FIFO_STATUS) & TX_FIFO_EMPTY)>>17)
#if (CONFIG_SOC_BK7271)
#define UART3_RX_FIFO_EMPTY_GET()       ((REG_READ(REG_UART3_FIFO_STATUS) & RX_FIFO_EMPTY)>>19)
#endif
#define UART2_RX_FIFO_EMPTY_GET()       ((REG_READ(REG_UART2_FIFO_STATUS) & RX_FIFO_EMPTY)>>19)
#define UART1_RX_FIFO_EMPTY_GET()       ((REG_READ(REG_UART1_FIFO_STATUS) & RX_FIFO_EMPTY)>>19)

/*******************************************************************************
* Function Declarations
*******************************************************************************/
extern UINT32 uart_sw_init(UINT8 ch);
extern UINT32 uart_sw_uninit(UINT8 ch);
extern void uart_fifo_flush(UINT8 ch);
extern void uart_hw_uninit(UINT8 ch);
extern void uart_reset(UINT8 ch);
extern UINT32 uart_write_fifo_frame(UINT8 ch, kfifo_ptr_t tx_ptr, UINT32 count);
extern UINT32 uart_read_fifo_frame(UINT8 ch, kfifo_ptr_t rx_ptr);
extern UINT32 uart1_open(UINT32 op_flag);
extern UINT32 uart1_close(void);
extern UINT32 uart1_read(char *user_buf, UINT32 count, UINT32 op_flag);
extern UINT32 uart1_write(char *user_buf, UINT32 count, UINT32 op_flag);
extern UINT32 uart1_ctrl(UINT32 cmd, void *parm);
extern UINT32 uart2_open(UINT32 op_flag);
extern UINT32 uart2_close(void);
extern UINT32 uart2_read(char *user_buf, UINT32 count, UINT32 op_flag);
extern UINT32 uart2_write(char *user_buf, UINT32 count, UINT32 op_flag);
extern UINT32 uart2_ctrl(UINT32 cmd, void *parm);
#if (CONFIG_SOC_BK7271)
extern UINT32 uart3_open(UINT32 op_flag);
extern UINT32 uart3_close(void);
extern UINT32 uart3_read(char *user_buf, UINT32 count, UINT32 op_flag);
extern UINT32 uart3_write(char *user_buf, UINT32 count, UINT32 op_flag);
extern UINT32 uart3_ctrl(UINT32 cmd, void *parm);
#endif
//extern int uart_read_byte(int uport);
//extern int uart_write_byte(int uport, char c);
extern int uart_rx_callback_set(int uport, uart_callback callback, void *param);
extern int uart_tx_fifo_needwr_callback_set(int uport, uart_callback callback, void *param);
extern int uart_tx_end_callback_set(int uport, uart_callback callback, void *param);
extern void uart_set_tx_stop_end_int(UINT8 uport, UINT8 set);
extern void uart_set_tx_fifo_needwr_int(UINT8 uport, UINT8 set);
//extern void uart_hw_set_change(UINT8 uport, bk_uart_config_t *uart_config);
#endif // _UART_H_
// eof

