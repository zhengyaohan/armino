#ifndef _I2C0_H_
#define _I2C0_H_

#ifdef  USE_FM_I2C
#if (CONFIG_SOC_BK7271)
#include "bk_uart.h"

#define I2C0_DEBUG              0
#if I2C0_DEBUG
#define I2C0_PRT                 os_printf
#define I2C0_WPRT                warning_prf
#define I2C0_EPRT                os_printf
#else
#define I2C0_PRT                 os_null_printf
#define I2C0_WPRT                os_null_printf
#define I2C0_EPRT                os_printf
#endif
#define I2C0_EPRT                os_printf
#define I2C0_DEBUG_PRINTF		 os_null_printf


#define I2C0_BASE_ADDR                         (0x0802200)
#define REG_I2C0_CONFIG                        (I2C0_BASE_ADDR + 4 * 0)
#define I2C0_ENSMB                             (1 << 0)
#define I2C0_STA                               (1 << 1)
#define I2C0_STO                               (1 << 2)
#define I2C0_ACK_TX                            (1 << 3)
#define I2C0_TX_MODE                           (1 << 4)
#define I2C0_FREQ_DIV_POSI                     (6)
#define I2C0_FREQ_DIV_MASK                     (0x3FF)
#define I2C0_SI                                (1 << 16)
#define I2C0_ACK_RX                            (1 << 17)
#define I2C0_ACK_REQ                           (1 << 18)
#define I2C0_BUSY                              (1 << 19)

#define REG_I2C0_DAT                           (I2C0_BASE_ADDR + 4 * 1)
#define I2C0_DAT_MASK                          (0xFF)

static UINT32 i2c0_open(UINT32 op_flag);
static UINT32 i2c0_close(void);
static UINT32 i2c0_read(char *user_buf, UINT32 count, UINT32 op_flag);
static UINT32 i2c0_write(char *user_buf, UINT32 count, UINT32 op_flag);
static UINT32 i2c0_ctrl(UINT32 cmd, void *param);
#endif
#endif
#endif  // _I2C0_H_

