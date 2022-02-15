#include "include.h"
#include "bk_arm_arch.h"
#include "i2c0.h"
#include "i2c_pub.h"
#include "bk_api_int.h"
#include "bk_icu.h"
#include "bk_gpio.h"
#include "bk_drv_model.h"
#include "bk_api_mem.h"
#include "bk_api_rtos.h"
#include "sys_driver.h"

#ifdef  USE_FM_I2C
#if (CONFIG_SOC_BK7271)
typedef struct i2c0_msg {
	UINT16 RegAddr;
	UINT16 RemainNum;
	UINT8 *pData;
	UINT8 SalveID;
	UINT8 AddrFlag;
	UINT8 TransDone;
	UINT8 ErrorNO;
	UINT8 TxMode;		//0: Read;  1: Write
	UINT8 AddrWidth;
} I2C0_MSG_ST, *I2C0_MSG_PTR;

static const DD_OPERATIONS i2c0_op = {
	i2c0_open,
	i2c0_close,
	i2c0_read,
	i2c0_write,
	i2c0_ctrl
};

volatile I2C0_MSG_ST gi2c0;

static void i2c0_set_ensmb(UINT32 enable)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	if (enable)
		reg_val |= I2C0_ENSMB;
	else
		reg_val &= ~I2C0_ENSMB;
	REG_WRITE(reg_addr, reg_val);
}

static void i2c0_set_smbus_sta(UINT32 enable)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	if (enable)
		reg_val |= I2C0_STA;
	else
		reg_val &= ~I2C0_STA;
	REG_WRITE(reg_addr, reg_val);
}

static void i2c0_set_smbus_stop(UINT32 enable)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	if (enable)
		reg_val |= I2C0_STO;
	else
		reg_val &= ~I2C0_STO;
	REG_WRITE(reg_addr, reg_val);
}

static void i2c0_set_smbus_ack_tx(UINT32 enable)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	if (enable)
		reg_val |= I2C0_ACK_TX;
	else
		reg_val &= ~I2C0_ACK_TX;
	REG_WRITE(reg_addr, reg_val);
}

static void i2c0_set_smbus_tx_mode(UINT32 enable)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	if (enable)
		reg_val |= I2C0_TX_MODE;
	else
		reg_val &= ~I2C0_TX_MODE;
	REG_WRITE(reg_addr, reg_val);
}

static void i2c0_set_freq_div(UINT32 div)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	reg_val = (reg_val & ~(I2C0_FREQ_DIV_MASK << I2C0_FREQ_DIV_POSI))
			  | ((div & I2C0_FREQ_DIV_MASK) << I2C0_FREQ_DIV_POSI);
	REG_WRITE(reg_addr, reg_val);
}

static UINT32 i2c0_get_smbus_interrupt(void)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	return (reg_val & I2C0_SI) ? 1 : 0;
}

static void i2c0_clear_smbus_interrupt(void)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	reg_val |= I2C0_SI;

	REG_WRITE(reg_addr, reg_val);
}

static UINT32 i2c0_get_ack_rx(void)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	return (reg_val & I2C0_ACK_RX) ? 1 : 0;
}

static UINT32 i2c0_get_ack_req(void)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	return (reg_val & I2C0_ACK_REQ) ? 1 : 0;
}

static UINT32 i2c0_get_smbus_busy(void)
{
	UINT32 reg_addr = REG_I2C0_CONFIG;
	UINT32 reg_val = REG_READ(reg_addr);

	return (reg_val & I2C0_BUSY) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
static void i2c0_gpio_config(void)
{
	UINT32 param;

	//param = GFUNC_MODE_I2C0;
	sddev_control(DD_DEV_TYPE_GPIO, CMD_GPIO_ENABLE_SECOND, &param);
}

static void i2c0_power_up(void)
{
	UINT32 param;
	param = PWD_FM_I2C_CLK_BIT;
	sddev_control(DD_DEV_TYPE_ICU, CMD_CLK_PWR_UP, &param);
}

static void i2c0_power_down(void)
{
	UINT32 param;
	param = PWD_FM_I2C_CLK_BIT;
	sddev_control(DD_DEV_TYPE_ICU, CMD_CLK_PWR_DOWN, &param);
}

static void i2c0_enable_interrupt(void)
{
	UINT32 param;
	param = (IRQ_FM_I2C_BIT);
	// sddev_control(DD_DEV_TYPE_ICU, CMD_ICU_INT_ENABLE, &param);
	(void)sys_drv_int_enable(param);
}

static void i2c0_disable_interrupt(void)
{
	UINT32 param;
	param = (IRQ_FM_I2C_BIT);
	// sddev_control(DD_DEV_TYPE_ICU, CMD_ICU_INT_DISABLE, &param);
	(void)sys_drv_int_disable(param);
}

static void i2c0_isr(void)
{
	unsigned long   i2c0_config;
	unsigned long   work_mode, ack_rx, sta, si;

	i2c0_config = REG_READ(REG_I2C0_CONFIG);
	si = i2c0_config & I2C0_SI;

	I2C0_PRT("i2c0_isr\r\n");
	I2C0_PRT("i2c0_isr: i2c0_config=0x%x\r\n", i2c0_config);

	if (!si) {   // not SMBUS/I2C Interrupt
		I2C0_EPRT("i2c0_isr not SI! i2c0_config = 0x%lx\r\n", i2c0_config);
		return;
	}

	ack_rx = i2c0_config & I2C0_ACK_RX;
	sta = i2c0_config & I2C0_STA;
	work_mode = (gi2c0.TxMode & 0x01) ? 0x00 : 0x01;

	i2c0_config &= (~I2C0_STA);
	i2c0_config &= (~I2C0_STO);
	I2C0_PRT("1: gi2c0.AddrFlag=0x%x\r\n", gi2c0.AddrFlag);
	switch (work_mode) {
	case 0x00: {    // master write
		if (!ack_rx) { // nack
			i2c0_config |= I2C0_STO;        // send stop
			gi2c0.TransDone = 1;
			break;
		}
		I2C0_PRT("2: gi2c0.AddrFlag=0x%x\r\n", gi2c0.AddrFlag);

		if (gi2c0.AddrFlag & 0x10) {    // all address bytes has been tx, now tx data
			if (gi2c0.RemainNum == 0) { // all data bytes has been tx, now send stop
				i2c0_config |= I2C0_STO;    // send stop
				gi2c0.TransDone = 1;
			} else {
				REG_WRITE(REG_I2C0_DAT, *gi2c0.pData);
				gi2c0.pData ++;
				gi2c0.RemainNum --;
			}
			break;
		}

		if ((gi2c0.AddrFlag & 0x08) == 0) {
			if (gi2c0.AddrWidth == ADDR_WIDTH_8) {
				REG_WRITE(REG_I2C0_DAT, (gi2c0.RegAddr & 0xFF));
				gi2c0.AddrFlag |= 0x13;
			} else if (gi2c0.AddrWidth == ADDR_WIDTH_16) {
				REG_WRITE(REG_I2C0_DAT, (gi2c0.RegAddr >> 8));
				gi2c0.AddrFlag |= 0x08;
			}
		} else {
			REG_WRITE(REG_I2C0_DAT, (gi2c0.RegAddr & 0xFF));
			gi2c0.AddrFlag |= 0x10;
		}
	}
	break;

	case 0x01: {    // master read
		if (((gi2c0.AddrFlag & 0x10) == 0) && (ack_rx == 0)) {    // NACK
			i2c0_config = i2c0_config | I2C0_STO;
			gi2c0.TransDone = 1;
			gi2c0.ErrorNO = 1;
			break;
		}

		if (sta && !ack_rx) {       // when tx address, we need ACK
			i2c0_config = i2c0_config | I2C0_STO;
			gi2c0.TransDone = 1;
			break;
		}

		if (gi2c0.AddrFlag & 0x10) { // all address has been tx, now rx data
			i2c0_config &= (~I2C0_TX_MODE);

			if (sta) {
				i2c0_config = i2c0_config | I2C0_ACK_TX;        // send ACK
				break;
			}

			*gi2c0.pData = REG_READ(REG_I2C0_DAT);
			gi2c0.pData ++;
			gi2c0.RemainNum --;

			if (gi2c0.RemainNum == 0) {
				i2c0_config = (i2c0_config & (~I2C0_ACK_TX)) | I2C0_STO;     // send NACK and STOP
				gi2c0.TransDone = 1;
			} else {
				i2c0_config = i2c0_config | I2C0_ACK_TX;    // send ACK
			}
			break;
		}

		if ((gi2c0.AddrFlag & 0x08) == 0) {     // inner address need to be tx
			if (gi2c0.AddrWidth == ADDR_WIDTH_8) {
				REG_WRITE(REG_I2C0_DAT, (gi2c0.RegAddr & 0xFF));
				gi2c0.AddrFlag = (gi2c0.AddrFlag & (~0x0B)) | 0x0A;
			} else if (gi2c0.AddrWidth == ADDR_WIDTH_16) {
				REG_WRITE(REG_I2C0_DAT, (gi2c0.RegAddr >> 8));
				gi2c0.AddrFlag = (gi2c0.AddrFlag & (~0x0B)) | 0x08;
			}
		} else if ((gi2c0.AddrFlag & 0x02) == 0) {
			REG_WRITE(REG_I2C0_DAT, (gi2c0.RegAddr & 0xFF));
			gi2c0.AddrFlag = (gi2c0.AddrFlag & (~0x0B)) | 0x0A;
		} else {                        // inner address has been tx
			i2c0_config |= I2C0_STA;
			REG_WRITE(REG_I2C0_DAT, (gi2c0.SalveID << 1) | 0x01);
			gi2c0.AddrFlag |= 0x13;
		}
	}
	break;

	default:        // by gwf
		break;
	}

	REG_WRITE(REG_I2C0_CONFIG, i2c0_config);
	REG_WRITE(REG_I2C0_CONFIG, i2c0_config & (~I2C0_SI));
}

static void i2c0_software_init(void)
{
	ddev_register_dev(I2C0_DEV_NAME, (DD_OPERATIONS *)&i2c0_op);
}

static void i2c0_hardware_init(void)
{
	/* register interrupt */
	bk_int_isr_register(INT_SRC_I2C0, i2c0_isr, NULL);

	/* clear all setting */
	REG_WRITE(REG_I2C0_CONFIG, 0);
}

void i2c0_init(void)
{
	os_memset((unsigned char *)&gi2c0, 0, sizeof(I2C0_MSG_ST));
	i2c0_software_init();
	i2c0_hardware_init();
}

void i2c0_exit(void)
{
	REG_WRITE(REG_I2C0_CONFIG, 0);

	ddev_unregister_dev(I2C0_DEV_NAME);
}

static UINT32 i2c0_open(UINT32 op_flag)
{
	os_printf("i2c0_open\r\n");
	if (op_flag)
		i2c0_set_freq_div(op_flag);
	else {
		i2c0_set_freq_div(I2C_CLK_DIVID(I2C_DEFAULT_BAUD));  // 400KHZ
	}

	i2c0_enable_interrupt();
	i2c0_power_up();
	i2c0_gpio_config();

	return I2C0_SUCCESS;
}

static UINT32 i2c0_close(void)
{
	os_printf("i2c0_close\r\n");
	i2c0_set_ensmb(0);
	i2c0_disable_interrupt();
	i2c0_power_down();

	return I2C0_SUCCESS;
}

static UINT32 i2c0_read(char *user_buf, UINT32 count, UINT32 op_flag)
{
	UINT32 reg, start_tick, cur_tick;
	I2C_OP_PTR i2c_op;
	GLOBAL_INT_DECLARATION();

	i2c_op = (I2C_OP_PTR)op_flag;

	I2C0_PRT("i2c0_read\r\n");

	I2C0_PRT("i2c0_read: i2c_op->salve_id = 0x%x, i2c_op->op_addr = 0x%x\r\n",
			 i2c_op->salve_id, i2c_op->op_addr);

	I2C0_PRT("i2c0_read: count = %d\r\n", count);

	I2C0_PRT("i2c0_read: gi2c0.TransDone = %d\r\n", gi2c0.TransDone);

	if (gi2c0.TransDone != 0)
	{
		return 0;
	}

	GLOBAL_INT_DISABLE();
	// write cycle, write the subaddr to device
	gi2c0.TxMode = 0;
	gi2c0.RegAddr = i2c_op->op_addr;
	gi2c0.RemainNum = count;
	gi2c0.pData = user_buf;
	gi2c0.SalveID = i2c_op->salve_id;
	gi2c0.AddrFlag = 0;
	gi2c0.TransDone = 0;
	gi2c0.ErrorNO = 0;
	gi2c0.AddrWidth = i2c_op->addr_width;

	reg = REG_READ(REG_I2C0_CONFIG);
	reg |= I2C0_TX_MODE | I2C0_ENSMB;// Set TXMODE | ENSMB
	REG_WRITE(REG_I2C0_CONFIG, reg);

	reg = ((i2c_op->salve_id & 0x7f) << 1) + 0;  // SET LSB 0
	REG_WRITE(REG_I2C0_DAT, reg);

	reg = REG_READ(REG_I2C0_CONFIG);
	reg |= I2C0_STA;// Set STA
	REG_WRITE(REG_I2C0_CONFIG, reg);
	GLOBAL_INT_RESTORE();

	start_tick = rtos_get_time();
	while(1)
	{
        if(gi2c0.TransDone == 0)
        {
            UINT32 past = 0;
            cur_tick = rtos_get_time();
            past = (cur_tick >= start_tick)? (cur_tick - start_tick) : (0xffffffffu - start_tick + cur_tick);
            if((past) > I2C_READ_WAIT_TIMEOUT)
            {
                gi2c1.ErrorNO = 1;
                break;
            }
            //rtos_delay_milliseconds(10);
        }
        else
        {
            break;
        }
    }

	GLOBAL_INT_DISABLE();
	gi2c0.TransDone = 0;
	GLOBAL_INT_RESTORE();

	return gi2c0.ErrorNO;
}

static UINT32 i2c0_write(char *user_buf, UINT32 count, UINT32 op_flag)
{
	UINT32 reg, start_tick, cur_tick;
	I2C_OP_PTR i2c_op;
	GLOBAL_INT_DECLARATION();

	i2c_op = (I2C_OP_PTR)op_flag;

	I2C0_PRT("i2c0_write\r\n");

	I2C0_PRT("i2c0_write: i2c_op->salve_id = 0x%x, i2c_op->op_addr = 0x%x\r\n",
			 i2c_op->salve_id, i2c_op->op_addr);

	I2C0_PRT("i2c0_write: count = %d\r\n", count);

	I2C0_PRT("i2c0_write: gi2c0.TransDone = %d\r\n", gi2c0.TransDone);

	if (gi2c0.TransDone != 0)
	{
		return 0;
	}

	GLOBAL_INT_DISABLE();
	gi2c0.TxMode = 1;
	gi2c0.RegAddr = i2c_op->op_addr;
	gi2c0.RemainNum = count;
	gi2c0.pData = (UINT8 *)user_buf;
	gi2c0.SalveID = i2c_op->salve_id;
	gi2c0.AddrFlag = 0;
	gi2c0.TransDone = 0;
	gi2c0.ErrorNO = 0;
	gi2c0.AddrWidth = i2c_op->addr_width;

	reg = REG_READ(REG_I2C0_CONFIG);
	reg |= I2C0_TX_MODE | I2C0_ENSMB;// Set TXMODE | ENSMB
	REG_WRITE(REG_I2C0_CONFIG, reg);

	reg = ((i2c_op->salve_id & 0x7f) << 1) + 0; // SET LSB 0
	REG_WRITE(REG_I2C0_DAT, reg);

	reg = REG_READ(REG_I2C0_CONFIG);
	reg |= I2C0_STA;// Set STA
	REG_WRITE(REG_I2C0_CONFIG, reg);
	GLOBAL_INT_RESTORE();

	start_tick = rtos_get_time();
	while(1)
	{
		if(gi2c0.TransDone == 0)
		{
			UINT32 past = 0;
			cur_tick = rtos_get_time();
			past = (cur_tick >= start_tick)? (cur_tick - start_tick) : (0xffffffffu - start_tick + cur_tick);
			if((past) > I2C_WRITE_WAIT_TIMEOUT)
			{
				gi2c1.ErrorNO = 1;
				break;
			}
			//rtos_delay_milliseconds(10);
		}
		else
		{
			break;
		}
	}

	GLOBAL_INT_DISABLE();
	gi2c0.TransDone = 0;
	GLOBAL_INT_RESTORE();

	return gi2c0.ErrorNO;
}

static UINT32 i2c0_ctrl(UINT32 cmd, void *param)
{
	UINT32 ret = I2C0_SUCCESS;

	switch (cmd) {
	case I2C0_CMD_SET_ENSMB:
		i2c0_set_ensmb(*((UINT32 *)param));
		break;
	case I2C0_CMD_SET_SMBUS_STA:
		i2c0_set_smbus_sta(*((UINT32 *)param));
		break;
	case I2C0_CMD_SET_SMBUS_STOP:
		i2c0_set_smbus_stop(*((UINT32 *)param));
		break;
	case I2C0_CMD_SET_SMBUS_ACK_TX:
		i2c0_set_smbus_ack_tx(*((UINT32 *)param));
		break;
	case I2C0_CMD_SET_SMBUS_TX_MODE:
		i2c0_set_smbus_tx_mode(*((UINT32 *)param));
		break;
	case I2C0_CMD_SET_FREQ_DIV:
		i2c0_set_freq_div(*((UINT32 *)param));
		break;
	case I2C0_CMD_GET_SMBUS_INTERRUPT:
		ret = i2c0_get_smbus_interrupt();
		break;
	case I2C0_CMD_CLEAR_SMBUS_INTERRUPT:
		i2c0_clear_smbus_interrupt();
		break;
	case I2C0_CMD_GET_ACK_RX:
		ret = i2c0_get_ack_rx();
		break;
	case I2C0_CMD_GET_ACK_REQ:
		ret = i2c0_get_ack_req();
		break;
	case I2C0_CMD_GET_SMBUS_BUSY:
		ret = i2c0_get_smbus_busy();
		break;

	default:
		break;
	}

	return ret;
}
#endif
#endif

